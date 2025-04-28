set(req driver freertos SSD1306)
set(priv_req I2C_SCAN component Lora_bitmap)  
idf_component_register(SRCS "lora_rx.c"
                      INCLUDE_DIRS "." "D:/Esp-idf/Esp32MCU_jigaku/components/SSD1306/src/intf"
                      REQUIRES ${req}
                      PRIV_REQUIRES ${priv_req})