#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lora.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "lora_logo.h"

//Oled hien thi thong so
#include "lcd/oled_ssd1306.h"
#include "ssd1306_generic.h"
#include "ssd1306_fonts.h"
#include "ssd1306_1bit.h"

#define SCL_PIN          22
#define SDA_PIN          21
#define LED_PIN          27
#define NORMAL_CONTRAST  UINT8_C(0x7F)
#define LORA_RX_FREQ     433e6 //433MHz
#define SSD1306_ADDR     INT8_C(0x3C) //Dia chi I2C chi co 7-bit

TaskHandle_t lora_rx_handle_task = NULL;
TaskHandle_t oled_display_handle_task = NULL;
uint8_t char_buf[256];

//Bien luu do dai goi tin
volatile int lastest_rx_len = 0;
volatile int lastest_rssi = 0;
volatile float lastest_snr = 0.0f;
volatile int lastest_cr = 0;
volatile bool isNewData = false;

void params_display(void){
  ssd1306_clearScreen();

  //In noi dung goi tin
  ssd1306_printFixed(0, 0, "Lora RX: ", STYLE_NORMAL);
  ssd1306_printFixed(0, 10, (char*)char_buf, STYLE_BOLD);    
  
  char line_buf[32];
  //Hien thi RSSI
  snprintf(line_buf, sizeof(line_buf), "RSSI: %d dBm", lastest_rssi);
  ssd1306_printFixed(0, 30, line_buf, STYLE_NORMAL);

  //Hien thi SNR 
  snprintf(line_buf, sizeof(line_buf), "SNR: %.1f dB", lastest_snr);
  ssd1306_printFixed(0, 40, line_buf, STYLE_NORMAL);

  //Hien thi CR
  snprintf(line_buf, sizeof(line_buf), "CR: %d", lastest_cr);
  ssd1306_printFixed(0, 60, line_buf, STYLE_NORMAL);
}

void __attribute__((unused))welcome_display(void){
  ssd1306_clearScreen();

  ssd1306_printFixed(35, 0, "HNL - TEAM", STYLE_BOLD);
  ssd1306_printFixed(20, 50, "LOW ENERGY LORA", STYLE_NORMAL);
  ssd1306_drawBitmap((128 - STAR_WIDTH) / 2, ((64 - STAR_HEIGHT) / 2) / 8, STAR_WIDTH, STAR_HEIGHT, star_logo);
  vTaskDelay(pdMS_TO_TICKS(2000));
} 

//Ham hien thi oled, neu trong 5s khong co tin hieu moi -> display off
void oled_display_task(void *pvParameter){
  const TickType_t timeout_ticks = pdMS_TO_TICKS(5000); 
  TickType_t last_update = xTaskGetTickCount();
  bool is_display_on = true;  

  while(true){
    if(isNewData){
      isNewData = false;
      params_display();
      last_update = xTaskGetTickCount();

      if(!is_display_on){ //Neu man hinh dang tat
        ssd1306_displayOn();
        is_display_on = true;
      }
    }else{
      if(is_display_on && (xTaskGetTickCount() - last_update > timeout_ticks)){
        ssd1306_displayOff();
        is_display_on = false;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

int oled_i2c_init(void){
  ssd1306_128x64_i2c_initEx(SCL_PIN, SDA_PIN, SSD1306_ADDR); //Dung ham nay thi khong scan duoc I2C (?)
  ssd1306_setContrast(NORMAL_CONTRAST);
  ssd1306_setFixedFont(ssd1306xled_font6x8);

  return 1;
}

void led_init(void){
  gpio_config_t led_gpio ={
    .mode = GPIO_MODE_OUTPUT, //Output mode
    .intr_type = GPIO_INTR_DISABLE,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .pin_bit_mask = (1ULL << LED_PIN),
  };
  gpio_config(&led_gpio);
}

void lora_rx(void *pvParameter){
  ESP_LOGI("LORA", "Receiving...!");
  while(1){
    lora_receive();
    if(lora_received()){
      lastest_rssi = lora_packet_rssi();
      lastest_snr = lora_packet_snr();
      lastest_cr = lora_get_coding_rate(); 
      lastest_rx_len = lora_receive_packet(char_buf, sizeof(char_buf));
      isNewData = true;

      printf("%d[%.*s],RSSI:%d,SNR:%f,CR:%d\n",lastest_rx_len, lastest_rx_len, char_buf, lastest_rssi, lastest_snr, lastest_cr);
      gpio_set_level(LED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(100));
      gpio_set_level(LED_PIN, LOW);
    }
    vTaskDelay(1);
  }
}

void lora_rx_init(){
  if(lora_init() == 0){
    ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module !");
    while(1){
      vTaskDelay(1);
    }
  }
  /**
   * @param coding_rate Tham so quy dinh muc do sua loi FEC 
   * + Do ben tin hieu (Chong nhieu, giam loi khi truyen xa)
   * + Toc do du lieu (CR cao -> toc do thap nhung dang tin cay hon)
   * + Khoang cach truyen (CR cao -> truyen xa hon nhung ton bang thong hon)
   */
  
  int coding_rate = 3; 
  int band_width = 7; 
  int spreading_factor = 12;

  #if CONFIG_ADVANCED 
    coding_rate = CONFIG_CODING_RATE;
    band_width = CONFIG_BANDWIDTH;
    spreading_factor = CONFIG_SF_RATE;
  #endif

  lora_enable_crc();

  lora_set_frequency(LORA_RX_FREQ); //Set tan so 
  ESP_LOGI(pcTaskGetName(NULL), "Frequency is 433MHz !");

  lora_set_coding_rate(coding_rate);
  ESP_LOGI(pcTaskGetName(NULL), "coding rate = %d", coding_rate);

  lora_set_bandwidth(band_width);
  ESP_LOGI(pcTaskGetName(NULL), "bandwidth = %d", band_width);

  lora_set_spreading_factor(spreading_factor);
  ESP_LOGI(pcTaskGetName(NULL), "spreading factor = %d", spreading_factor);

  vTaskDelay(pdMS_TO_TICKS(10));
}
  
void app_main(void){  
  lora_rx_init();
  led_init();

  const int err = oled_i2c_init(); ///Khoi tao cho man hinh i2c
  if(err == 1){
    ESP_LOGI(pcTaskGetName(NULL), "OLED intialized OK !");
    ssd1306_displayOn();
    welcome_display();
  }

  xTaskCreatePinnedToCore(lora_rx, "lora receiver", 1024 * 3, NULL, 1, &lora_rx_handle_task, 0);
  xTaskCreatePinnedToCore(oled_display_task, "oled display", 1024 * 3, NULL, 1, &oled_display_handle_task, 1);
}