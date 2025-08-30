idf_component_register(
    SRCS "node_id.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_common esp_hw_support efuse esp_system
)
