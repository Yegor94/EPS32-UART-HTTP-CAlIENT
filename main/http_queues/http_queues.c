/*
 * http_queues.c
 *
 *  Created on: 26 ма€ 2021 г.
 *      Author: ≈гор
 */

#include "http_queues/http_queues.h"



QueueHandle_t RxQueue = NULL;
QueueHandle_t TxQueue = NULL;

TaskHandle_t Uart_parser_task_handle = NULL,
		uart_tx_task_handle = NULL,
		http_client_task_handle = NULL,
		send_to_tx_queue_task_handle = NULL;


uint8_t tx_queue_pop_front(QueueHandle_t* p_queue_handle)
{
	http_data_t* p_data = NULL;
	SemaphoreHandle_t xQueueSemaphore = xSemaphoreCreateMutex();
	if(xSemaphoreTake(xQueueSemaphore, (TickType_t)10) == pdTRUE){
		if(xQueueGenericReceive(*p_queue_handle, &(p_data), (TickType_t)10, pdFALSE)){
			free(p_data);
			xSemaphoreGive(xQueueSemaphore);
			return 0;
		}
		else{
			xSemaphoreGive(xQueueSemaphore);
			return -1;
		}
	}else return -1;
}

uint8_t rx_queue_pop_front(QueueHandle_t* p_queue_handle)
{
	data_for_uart_tx_t* p_data = NULL;
	SemaphoreHandle_t xQueueSemaphore = xSemaphoreCreateMutex();
	if(xSemaphoreTake(xQueueSemaphore, (TickType_t)10) == pdTRUE){
		if(xQueueGenericReceive(*p_queue_handle, &(p_data), (TickType_t)10, pdFALSE)){
			free(p_data);
			xSemaphoreGive(xQueueSemaphore);
			return 0;
		}
		else{
			xSemaphoreGive(xQueueSemaphore);
			return -1;
		}
	}else return -1;
}
