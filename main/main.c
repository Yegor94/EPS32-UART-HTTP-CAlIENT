
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "lwip/netif.h"
#include "protocol_examples_common.h"
#include "esp_tls.h"
#include "esp_task_wdt.h"
#include "esp_int_wdt.h"
#include "freertos/FreeRTOSConfig.h"

#include "uart/uart_task.h"
#include "http_client/http_client.h"
#include "http_queues/http_queues.h"


void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    TxQueue = xQueueCreate(TX_QUEUE_SIZE, TX_ITEM_SIZE);
    RxQueue = xQueueCreate(RX_QUEUE_SIZE, RX_ITEM_SIZE);

    if(TxQueue == NULL || RxQueue == NULL){
    	ESP_LOGI("main", "Queu was't created!");
    	esp_restart();
    }

    ESP_ERROR_CHECK(esp_task_wdt_init(5, true));

    uart_init();
}
