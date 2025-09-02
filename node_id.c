#include "node_id.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

// Base32 Crockford alphabet (no I, L, O, U)
static const char *B32 = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

static const char *NODE_ID_NAMESPACE = "PH_NODE_ID"; // Photon Circus Node ID namespace to prevent collision

// Base32 Crockford encoded node ID with checksum: "XXXX-XXXC"
static char s_node_id[NODE_ID_MAX_LEN] = {0};
static bool s_node_id_initialized = false;

#define NODE_MAC_ADDR_STRLEN 18

#ifdef ESP_PLATFORM

#include "esp_log.h"
#include "esp_mac.h"
#include "mbedtls/sha256.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t s_node_id_mutex = NULL;

static ERROR_TYPE hash_sha256(const uint8_t *data, size_t len, uint8_t *out, size_t *out_len)
{
    if (!data || !out || !out_len)
    {
        return ERROR_INVALID_ARG;
    }
    if (*out_len < 32)
    {
        return ERROR_INVALID_ARG;
    }

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, data, len);
    mbedtls_sha256_finish(&ctx, out);
    mbedtls_sha256_free(&ctx);

    return ERROR_OK;
}

static ERROR_TYPE get_node_mac(char *out)
{
    uint8_t mac[6] = {0};
    if (esp_efuse_mac_get_default(mac) != ESP_OK)
    {
        return ERROR_FAIL;
    }
    snprintf(out, NODE_MAC_ADDR_STRLEN, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);
    return ERROR_OK;
}

#else
#error Only esp_idf supported
#endif

static uint8_t mod37_check(const uint8_t *bytes, size_t len)
{
    // Simple mod-37 checksum over the underlying hash bytes (maps to 0..36)
    uint32_t s = 0;
    for (size_t i = 0; i < len; i++)
        s = (s + bytes[i]) % 37;
    return (uint8_t)s;
}

static void b32_64bits_to_13(const uint8_t h[8], char out[14])
{
    // 64 bits -> 13 symbols (since 13*5 = 65 >= 64)
    uint64_t v = ((uint64_t)h[0] << 56) | ((uint64_t)h[1] << 48) | ((uint64_t)h[2] << 40) | ((uint64_t)h[3] << 32) |
                 ((uint64_t)h[4] << 24) | ((uint64_t)h[5] << 16) | ((uint64_t)h[6] << 8) | ((uint64_t)h[7]);
    for (int i = 12; i >= 0; i--)
    {
        out[i] = B32[v & 0x1F];
        v >>= 5;
    }
    out[13] = '\0';
}

static void apply_format_and_store(const char core13[14], uint8_t check)
{
    // "XXXXX-XXXXX-XXX" + "-" + check char => up to 18-19 chars
    // group 5-5-3 for readability
    char grouped[18] = {0};
    memcpy(&grouped[0], &core13[0], 5);
    grouped[5] = '-';
    memcpy(&grouped[6], &core13[5], 5);
    grouped[11] = '-';
    memcpy(&grouped[12], &core13[10], 3);
    grouped[15] = '-';
    grouped[16] = B32[check % 32];
    grouped[17] = '\0';
    strncpy(s_node_id, grouped, NODE_ID_MAX_LEN);
}

static ERROR_TYPE make_id(const uint8_t *identity_str, size_t len)
{
    char *buffer = (char *)malloc(len + strlen(NODE_ID_NAMESPACE));
    if (!buffer)
        return ERROR_NO_MEM;

    memcpy(buffer, NODE_ID_NAMESPACE, strlen(NODE_ID_NAMESPACE));
    memcpy(buffer + strlen(NODE_ID_NAMESPACE), identity_str, len);

    uint8_t hash[32] = {0};
    size_t hash_len = 32;
    if (hash_sha256((const uint8_t *)buffer, len + strlen(NODE_ID_NAMESPACE), hash, &hash_len) != ERROR_OK)
    {
        free(buffer);
        return ERROR_NO_MEM;
    }
    free(buffer);

    char core13[14];
    b32_64bits_to_13(hash, core13);
    uint8_t chk = mod37_check(hash, 8);

    apply_format_and_store(core13, chk);

    return ERROR_OK;
}


// Internal helper: assumes mutex is held
static ERROR_TYPE node_id_init_locked(const uint8_t *identity_str, size_t len, bool force)
{
    if (!identity_str)
    {
        char mac_str[NODE_MAC_ADDR_STRLEN] = {0};
        if (get_node_mac(mac_str) != ERROR_OK)
        {
            return ERROR_FAIL;
        }
        identity_str = (uint8_t *)mac_str;
        len = strlen(mac_str);
    }

    if (s_node_id_initialized && !force)
        return ERROR_OK;

    if (make_id(identity_str, len) != ERROR_OK)
    {
        return ERROR_FAIL;
    }

    s_node_id_initialized = true;
    return ERROR_OK;
}

ERROR_TYPE node_id_init(const uint8_t *identity_str, size_t len)
{
    // Initialize mutex once
    if (s_node_id_mutex == NULL)
    {
        SemaphoreHandle_t m = xSemaphoreCreateMutex();
        if (m == NULL)
        {
            return ERROR_NO_MEM;
        }
        if (s_node_id_mutex == NULL)
            s_node_id_mutex = m;
        else
            vSemaphoreDelete(m);
    }

    if (xSemaphoreTake(s_node_id_mutex, portMAX_DELAY) != pdTRUE)
        return ERROR_FAIL;

    ERROR_TYPE result = node_id_init_locked(identity_str, len, false);
    xSemaphoreGive(s_node_id_mutex);
    return result;
}

ERROR_TYPE node_id_force_init(const uint8_t *identity_str, size_t len)
{
    if (!identity_str || len == 0)
        return ERROR_INVALID_ARG;

    if (s_node_id_mutex == NULL)
    {
        SemaphoreHandle_t m = xSemaphoreCreateMutex();
        if (m == NULL)
            return ERROR_NO_MEM;
        if (s_node_id_mutex == NULL)
            s_node_id_mutex = m;
        else
            vSemaphoreDelete(m);
    }

    if (xSemaphoreTake(s_node_id_mutex, portMAX_DELAY) != pdTRUE)
        return ERROR_FAIL;

    // Warn about buffer overwrite
    #ifdef ESP_PLATFORM
    ESP_LOGW("node_id", "node_id_force_init: Overwriting static node ID buffer. Unsafe if other modules hold pointer.");
    #endif

    // Save previous ID
    char prev_id[NODE_ID_MAX_LEN];
    strncpy(prev_id, s_node_id, NODE_ID_MAX_LEN);
    prev_id[NODE_ID_MAX_LEN - 1] = '\0';

    ERROR_TYPE result = node_id_init_locked(identity_str, len, true);

    // If new ID is different, log warning
    if (strncmp(prev_id, s_node_id, NODE_ID_MAX_LEN) != 0) {
        #ifdef ESP_PLATFORM
        ESP_LOGW("node_id", "node_id_force_init: Node ID buffer changed from '%s' to '%s'", prev_id, s_node_id);
        #endif
    }

    xSemaphoreGive(s_node_id_mutex);
    return result;
}

ERROR_TYPE get_node_id(char **node_id, size_t *len)
{
    if (!node_id || !len)
    {
        return ERROR_INVALID_ARG;
    }

    if (!s_node_id_initialized)
    {
        if (node_id_init(NULL, 0) != ERROR_OK)
        {
            return ERROR_FAIL;
        }
    }

    *node_id = s_node_id;
    *len = strlen(s_node_id);
    return ERROR_OK;
}
