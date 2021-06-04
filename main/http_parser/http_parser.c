/*
 * http_parser.c
 *
 *  Created on: 26 ма€ 2021 г.
 *      Author: ≈гор
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

#include "http_parser/http_parser.h"
#include "http_queues/http_queues.h"
#include "uart/uart_task.h"

SemaphoreHandle_t xParserSemaphore = NULL;

static int parseMessage(http_data_t* p_data_for_parse)
{
	char http_keyword[6] = "http ",
		 POST_keyword[6] = "POST ",
		 GET_keyword[5] = "GET ";
	taskYIELD();
	if(memcmp((void*)http_keyword, (void*)p_data_for_parse->buffer, 5)){ // does keyword "http" present
		send_to_uart("Error! Bad parameters (http)!\n");
		for(uint8_t i=0; i<5; i++){
			if(!tx_queue_pop_front(&TxQueue)){ // Delete wrong request from TxQueue
				break;
			}
			else vTaskDelay(1/portTICK_PERIOD_MS);
		}
		return 1;
	}

	if(!memcmp((void*)POST_keyword, (void*)(&p_data_for_parse->buffer[5]), 5)){ // does keyword "POST" present
		p_data_for_parse->request_type = POST;
	}
	else if(!memcmp(GET_keyword, (void*)(&p_data_for_parse->buffer[5]), 4)){ // does keyword "GET" present
		p_data_for_parse->request_type = GET;
	}
	else
	{
		send_to_uart("Error! Bad parameters (request type)!\n");
		for(uint8_t i=0; i<5; i++){
			if(!tx_queue_pop_front(&TxQueue)){ // Delete wrong request from TxQueue
				break;
			}
			else vTaskDelay(1/portTICK_PERIOD_MS);
		}
		return 1;
	}
	for(uint16_t i=p_data_for_parse->request_type == 1 ? 8 : 9; // Getting position of start http address and start of POST request body
			i<p_data_for_parse->recieving_stopped_pos;
			i++){
		if(p_data_for_parse->buffer[i] == ' ')
		{
			if(p_data_for_parse->http_addr_start_pos == 0){
				p_data_for_parse->http_addr_start_pos = i+1;
				continue;
			}
			if(p_data_for_parse->request_type == 2 &&
					p_data_for_parse->http_addr_start_pos != 0)
			{
				p_data_for_parse->post_body_start_pos = i+1;
				return 0;
			}
		}
	}
	return 0;
}

void parserTask(void* pvParameters)
{
	xParserSemaphore = xSemaphoreCreateMutex();
	if(xParserSemaphore == NULL) ESP_LOGI("parserTask", "xParserSemaphore do not created");
	http_data_t* p_data_to_send = NULL;

	while(1){
		if(uxQueueSpacesAvailable(TxQueue) == TX_QUEUE_SIZE) {
			break;
		}
		if(xSemaphoreTake(xParserSemaphore, (TickType_t)10) == pdTRUE)
		{
			if(xQueueGenericReceive(TxQueue, &(p_data_to_send), (TickType_t)10, pdTRUE)){ // Read item from queue but don't delete it
				xSemaphoreGive(xParserSemaphore);

				if(p_data_to_send->http_addr_start_pos)
					vTaskSuspend(NULL); // Suspend task when first item in TxQueue was already processed but steel not sent to http server

				if(!parseMessage(p_data_to_send)){
					if(http_client_task_handle == NULL){
						xTaskCreate(http_client_task, "http_client_task", 6144, NULL, 7, http_client_task_handle);
					}
					else vTaskResume(http_client_task_handle);
				}
			}
		}
		else ESP_LOGE("parserTask", "Can't take a semaphore"); // Creatre deley for 1 ms
	}
	vTaskDelete(NULL);
}




