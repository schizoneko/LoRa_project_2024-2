#include <stdio.h>
#include <stdint.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lora.h"
#include "sdkconfig.h"

//Do tang ich thi tinh dinh huong cao
#define LORA_TX_FREQ 433e6
TaskHandle_t lora_tx_handle_task = NULL;
uint8_t char_buf[256];

void lora_tx(void *pvParameters){
  ESP_LOGI(pcTaskGetName(NULL), "Tasking...");

  while(true){
    TickType_t nowTick = xTaskGetTickCount(); //Tra ve so tick ke tu khi he thong khoi dong (1 tick = 1ms neu cau hinh he thong la 1000Hz)
    int send_len = sprintf((char*)char_buf, "Hello World %"PRIu32, nowTick); //Ghi vao chuoi ky tu char_buf dang [Hello World!!! +ms]
    lora_send_packet(char_buf, send_len);

    ESP_LOGI(pcTaskGetName(NULL), "%d bytes packet sent, tx_time: %"PRIu32, send_len, nowTick);
    vTaskDelay(pdMS_TO_TICKS(2000)); //Delay 2s roi gui tiep (1s la loi data vi time xu ly ngan)
  } 
}

void lora_tx_init(){
  if(lora_init() == 0){
    ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module !");
    while(1){
      vTaskDelay(1);
    }
  }
  /**
   * @param coding_rate Tham so quy dinh muc do sua loi FEC 
   * \note + Do ben tin hieu (Chong nhieu, giam loi khi truyen xa)
   * \note + Toc do du lieu (CR cao -> toc do thap nhung dang tin cay hon)
   * \note + Khoang cach truyen (CR cao -> truyen xa hon nhung ton bang thong hon)
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

  lora_set_frequency(LORA_TX_FREQ); //Set tan so 
  ESP_LOGI(pcTaskGetName(NULL), "Frequency is 433MHz !");

  lora_set_coding_rate(coding_rate);
  ESP_LOGI(pcTaskGetName(NULL), "coding rate = %d", coding_rate);

  lora_set_bandwidth(band_width);
  ESP_LOGI(pcTaskGetName(NULL), "bandwidth = %d", band_width);

  lora_set_spreading_factor(spreading_factor);
  ESP_LOGI(pcTaskGetName(NULL), "spreading factor = %d", spreading_factor);

  lora_set_tx_power(17);
  ESP_LOGI(pcTaskGetName(NULL), "Power trasmitter = %d", 17);
}

void app_main(void){
  lora_tx_init();
  xTaskCreatePinnedToCore(lora_tx, "lora transmitter", 1024 *3 , NULL, 1, &lora_tx_handle_task, 0);
} 