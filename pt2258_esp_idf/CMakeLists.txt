idf_component_register(
    SRCS "pt2258_esp_idf.c"
    INCLUDE_DIRS "include"
    REQUIRES
        "pt2258"
        "esp_driver_i2c"
)