/*
 * http_client.h
 *
 *  Created on: 26 ма€ 2021 г.
 *      Author: ≈гор
 */

#ifndef MAIN_HTTP_CLIENT_HTTP_CLIENT_H_
#define MAIN_HTTP_CLIENT_HTTP_CLIENT_H_

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "lwip/netif.h"
#include "protocol_examples_common.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "esp_task_wdt.h"

#include "uart/uart_task.h"
#include "http_queues/http_queues.h"
#include "http_parser/http_parser.h"

/*
 * http_client_task(void *pvParameters) - take first item from TX queue,
 * send request to http server and receive response from server. After that
 * delete item which was sent to server from TX queue.
 * Then push bsck response to RX queue.
 * If TX queue is empty task will delete himselve from FreeRTOS task list.
 */
void http_client_task(void *pvParameters);


#endif /* MAIN_HTTP_CLIENT_HTTP_CLIENT_H_ */
