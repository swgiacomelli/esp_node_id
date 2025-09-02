#include <stdio.h>
#include <string.h>

/**
 * Example usage of the node_id ESP-IDF component.
 *
 * - The node ID is a short, human-friendly string derived from the device's MAC address by default.
 * - You can optionally provide a custom identity (e.g., public key) to generate a deterministic node ID.
 * - The API is thread-safe; the returned pointer is always to an internal static buffer (never freed).
 * - WARNING: Forcing re-initialization (node_id_force_init) is unsafe if other modules are using the node ID.
 *   It will overwrite the internal buffer and may break consumers holding a pointer.
 */

#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "node_id.h"

static const char *TAG = "example";

void app_main(void)
{
    // Example custom identity (could be a public key, serial number, etc.)
    uint8_t custom_id[] = "CUSTOM_IDENTITY";

    char *id = NULL;
    size_t len = 0;

    // Get the default node ID (derived from MAC address)
    if (get_node_id(&id, &len) == ESP_OK)
    {
        ESP_LOGI(TAG, "Node ID (%d): %s", (int)len, id);

        // Save the old node ID before force init (since get_node_id returns a pointer to a static buffer)
        char old_id[NODE_ID_MAX_LEN];
        strncpy(old_id, id, NODE_ID_MAX_LEN);
        old_id[NODE_ID_MAX_LEN - 1] = '\0';

        // Force re-initialization with a custom identity
        ESP_LOGI(TAG, "Forcing custom Node ID initialization with identity: %s", custom_id);
        ESP_LOGI(TAG, "Custom Node ID (%d): %s", (int)sizeof(custom_id), custom_id);

        if (node_id_force_init(custom_id, sizeof(custom_id) - 1) == ESP_OK)
        {
            ESP_LOGI(TAG, "Node ID set to custom identity");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to set custom node id");
        }

        // Get the new node ID after force init
        if (get_node_id(&id, &len) == ESP_OK)
        {
            ESP_LOGI(TAG, "New Node ID (%d): %s", (int)len, id);
            ESP_LOGI(TAG, "Old Node ID (%d): %s", (int)strlen(old_id), old_id);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to get node id");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get node id");
    }
}
