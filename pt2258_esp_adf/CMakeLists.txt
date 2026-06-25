idf_component_register(
    SRCS "pt2258_esp_adf.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        "esp_peripherals"
        "pt2258"
)