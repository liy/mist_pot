idf_component_register(SRCS "main.c"
    PRIV_REQUIRES spi_flash driver esp_driver_ledc esp_adc
    INCLUDE_DIRS "")
