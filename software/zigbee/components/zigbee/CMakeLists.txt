idf_component_register(SRC_DIRS "src"
                       INCLUDE_DIRS "include"
                       REQUIRES "freertos" "nvs_flash" "driver" "human_interface" "zigbee_electricity_meter"
)