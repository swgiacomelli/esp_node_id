#include <stdio.h>
#include "esp_log.h"
#include "node_id.h"

static const char *TAG = "example";

void app_main(void)
{
    char *id = NULL;
    size_t len = 0;
    if (get_node_id(&id, &len) == ESP_OK)
    {
        ESP_LOGI(TAG, "Node ID (%d): %s", (int)len, id);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get node id");
    }
}
