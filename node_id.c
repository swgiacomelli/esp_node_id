// Public header for this component
#include "node_id.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_crc.h"
#include "esp_mac.h"

/**
 * Node ID format and buffer sizing.
 *
 * Final string shape: "XXXX-XXXC" (9 visible chars + null terminator)
 * - 7 chars are Base32 (Crockford) encoding of a CRC32 over the factory MAC
 * - The last char is a 5-bit checksum derived from those 7 chars
 */
#define NODE_ID_PAYLOAD_LEN   7
#define NODE_ID_STR_LEN       9
#define NODE_ID_BUF_LEN       (NODE_ID_STR_LEN + 1)

// Base32 Crockford alphabet (no I, L, O, U)
static const char s_crock32[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

// Compile-time sanity: ensure alphabet has 32 symbols
_Static_assert(sizeof(s_crock32) - 1 == 32, "Crockford alphabet must have 32 symbols");

// Base32 Crockford encoded node ID with checksum: "XXXX-XXXC"
static char s_node_id[NODE_ID_BUF_LEN] = {0};
static bool s_node_id_initialized = false;

// Map 5-bit value to Crockford char
static inline char crock32_encode5(uint8_t v)
{
    return s_crock32[v & 0x1F];
}

// Encode 32-bit value into exactly 7 Crockford chars (MSB zero-padded)
static void crock32_encode_u32_7(uint32_t x, char out[NODE_ID_PAYLOAD_LEN])
{
    // 7 chars * 5 bits = 35 bits (but we only have 32; top 3 bits are zero)
    for (int i = NODE_ID_PAYLOAD_LEN - 1; i >= 0; --i) {
        out[i] = crock32_encode5((uint8_t)(x & 0x1F));
        x >>= 5;
    }
}

// CRC-8 (poly 0x31, init 0xFF) over payload chars, folded to 5 bits (1 symbol)
static uint8_t checksum5_over_payload(const char *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint8_t)data[i];
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    // Fold to 5 bits; ensure non-zero to avoid visually empty checksum
    uint8_t folded = (uint8_t)((crc ^ (crc >> 5) ^ (crc >> 3)) & 0x1F);
    return (folded == 0) ? 1 : folded;
}

// Build the cached node ID string once
static esp_err_t node_id_make(void)
{
    if (s_node_id_initialized) {
        return ESP_OK;
    }

    // Read factory MAC (48-bit)
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);

    // CRC-32 over 6 bytes (IDF uses little-endian variant)
    uint32_t crc = 0xFFFFFFFFu;
    crc = esp_crc32_le(crc, mac, sizeof(mac));
    crc ^= 0xFFFFFFFFu;

    // Encode 32-bit → 7 Crockford chars (not null-terminated)
    char payload[NODE_ID_PAYLOAD_LEN];
    crock32_encode_u32_7(crc, payload);

    // Compute 1-char checksum from payload
    const uint8_t check5 = checksum5_over_payload(payload, NODE_ID_PAYLOAD_LEN);
    const char check_ch = crock32_encode5(check5);

    // Assemble final string: AAAA-BBBC
    memcpy(&s_node_id[0], &payload[0], 4);
    s_node_id[4] = '-';
    memcpy(&s_node_id[5], &payload[4], 3);
    s_node_id[8] = check_ch;
    s_node_id[9] = '\0';

    s_node_id_initialized = true;
    return ESP_OK;
}

/**
 * Retrieve the unique node ID string.
 *
 * Outputs a pointer to a static, null-terminated buffer that remains valid for the
 * duration of the program. Thread-safety: initialization is not guarded; if multiple
 * tasks call this simultaneously on the first use, they may race during construction,
 * but the final value will be consistent thereafter.
 */
esp_err_t get_node_id(char **node_id, size_t *len)
{
    if (!node_id || !len) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = node_id_make();
    if (err != ESP_OK) {
        return err;
    }

    *node_id = s_node_id;
    *len = strlen(s_node_id);
    return ESP_OK;
}