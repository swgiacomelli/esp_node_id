#ifndef NODE_ID_H
#define NODE_ID_H

#include <stddef.h>
#include <stdint.h>

#ifdef PLATFORM_ESP
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define ERROR_TYPE esp_err_t
#define ERROR_OK ESP_OK
#define ERROR_INVALID_ARG ESP_ERR_INVALID_ARG
#define ERROR_NO_MEM ESP_ERR_NO_MEM
#define ERROR ESP_FAIL
#endif

#ifndef ERROR_TYPE
#define ERROR_TYPE uint32_t
#define ERROR_OK 0
#define ERROR_INVALID_ARG 1
#define ERROR_NO_MEM 2
#define ERROR_FAIL 3
#endif

#define NODE_ID_MAX_LEN 20 // Format: "XXXXX-XXXXX-XXX-C", where
                           // 5 digits + 1 dash + 5 digits + 1 dash + 3 digits + 1 dash + 1 check char + 1 null terminator
                           // = 5 + 1 + 5 + 1 + 3 + 1 + 1 + 1 = 18 (actual string) + 1 (check char) + 1 (null terminator) = 20

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initialize the node ID (thread-safe).
     * If identity_str is NULL, falls back to MAC address.
     * Returns pointer to static buffer, never freed.
     */
    ERROR_TYPE node_id_init(const uint8_t *identity_str, size_t len);

    /**
     * Force re-initialization of the node ID (thread-safe, but unsafe for consumers holding pointer).
     * Overwrites static buffer if identity changes. Use with caution.
     */
    ERROR_TYPE node_id_force_init(const uint8_t *identity_str, size_t len);

    /**
     * Get the current node ID string and its length.
     * The returned pointer is to a static buffer valid for the process lifetime.
     */
    ERROR_TYPE get_node_id(char **node_id, size_t *len);

#ifdef __cplusplus
}
#endif

#endif