/*
 * uart_task.c
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
#include "driver/uart.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "esp_task_wdt.h"


#if CONFIG_IDF_TARGET_ESP32
    #include "esp32/rom/uart.h"
#elif CONFIG_IDF_TARGET_ESP32S2
    #include "esp32s2/rom/uart.h"
#endif

#include "uart/uart_task.h"
#include "http_queues/http_queues.h"
#include "http_parser/http_parser.h"

// Receive buffer to collect incoming data
static uint8_t uart_rx_buf[BUFFER_SIZE] = {0};
// Register to collect data length
uint16_t urxlen = 0;

static intr_handle_t handle_console;
SemaphoreHandle_t xUartSemaphore = NULL;


/*
 * void IRAM_ATTR uart_intr_handle(void *arg) - interrupt handler
 * which receive characters from UART FIFO and put them to uart_rx_buf;
 * When we receive new line character '\n' variable urxlen assign 0 and start
 * send_to_tx_queue_task();
 */
static void IRAM_ATTR uart_intr_handle(void *arg)
{
	uint16_t rx_fifo_len;
	uint32_t status;
	status = UART0.int_st.val; // read UART interrupt Status
	rx_fifo_len = UART0.status.rxfifo_cnt; // read number of bytes in UART buffer

	urxlen = (urxlen == BUFFER_SIZE-1) ? 0 : urxlen; // Protect from uart rx buffer overflow

	while(rx_fifo_len){
		uart_rx_buf[urxlen++] = UART0.fifo.rw_byte; // read all bytes
		rx_fifo_len--;
	}

	if(status & UART_AT_CMD_CHAR_DET_INT_CLR){ // symbol end of line detected
		uart_rx_buf[urxlen-2] = 0; // Ignore symbol of new line '\n'
		urxlen=0;
		if(send_to_tx_queue_task_handle == NULL)
			xTaskCreate(send_to_tx_queue_task, "send_to_tx_queue_task", 2048, NULL, 5, send_to_tx_queue_task_handle);
		else
			xTaskResumeFromISR(send_to_tx_queue_task_handle);
	}
	uart_clear_intr_status(EX_UART_NUM, UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR|UART_AT_CMD_CHAR_DET_INT_CLR);
}


void uart_init()
{
	uart_config_t uart_config = {
			.baud_rate = UART_TASK_BOUDE_RATE,
			.data_bits = UART_TASK_DATA_BITS,
			.parity = UART_TASK_PARITY,
			.stop_bits = UART_TASK_STOP_BITS,
			.flow_ctrl = UART_TASK_HW_FLOWCTRL
		};

		ESP_ERROR_CHECK(uart_param_config(EX_UART_NUM, &uart_config));

		//Set UART pins (using UART0 default pins ie no changes.)
		ESP_ERROR_CHECK(uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

		//Install UART driver, and get the queue.
		ESP_ERROR_CHECK(uart_driver_install(EX_UART_NUM, BUFFER_SIZE*2, BUFFER_SIZE*2, 0, NULL, 0));

		// release the pre registered UART handler/subroutine
		ESP_ERROR_CHECK(uart_isr_free(EX_UART_NUM));

		// register new UART subroutine
		ESP_ERROR_CHECK(uart_isr_register(EX_UART_NUM,uart_intr_handle, NULL, ESP_INTR_FLAG_IRAM, &handle_console));
		uart_enable_pattern_det_intr(EX_UART_NUM, END_OF_LINE_SYMB, 1, 9, 0, 0);

		// enable RX interrupt
		ESP_ERROR_CHECK(uart_enable_rx_intr(EX_UART_NUM));

		xUartSemaphore = xSemaphoreCreateMutex();
		if(xUartSemaphore == NULL) ESP_LOGI("UART", "xUartSemaphore do not created");
}


void send_to_uart(char* data)
{
	data_for_uart_tx_t* p_recived_data = NULL;

	if(uxQueueSpacesAvailable(RxQueue) == 0){
		vTaskDelay(10/portTICK_PERIOD_MS);
	}
	else{
		p_recived_data = (data_for_uart_tx_t*)malloc(1*sizeof(data_for_uart_tx_t));
		p_recived_data->data_len = strlen(data);

		memcpy((char*)p_recived_data->buffer, data, p_recived_data->data_len);
		p_recived_data->buffer[p_recived_data->data_len] = '\0';

		if(xSemaphoreTake(xUartSemaphore, (TickType_t)10) == pdTRUE){
			xQueueGenericSend(RxQueue, ( void * ) &p_recived_data, (TickType_t)10, queueSEND_TO_BACK);
			xSemaphoreGive(xUartSemaphore);
		}
	}
	if(uart_tx_task_handle == NULL){
		xTaskCreate(uart_tx_task, "uart_tx_task", 6144, NULL, 1, uart_tx_task_handle);
	}
	else{
		vTaskResume(uart_tx_task_handle);
	}
}


void uart_tx_task(void *pvParameters)
{
	data_for_uart_tx_t* p_data_to_send = NULL;

	TickType_t xLastWakeTime;
	const TickType_t xFrequency = 100/portTICK_PERIOD_MS;
	xLastWakeTime = xTaskGetTickCount ();
	ESP_ERROR_CHECK(esp_task_wdt_add(uart_tx_task_handle));

	while(1){
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
		if(uxQueueSpacesAvailable(RxQueue) == RX_QUEUE_SIZE){
			break;
		}
		if(xSemaphoreTake(xUartSemaphore, (TickType_t)10) == pdTRUE)
		{
			if(xQueueGenericReceive(RxQueue, &(p_data_to_send), (TickType_t)10, pdTRUE)){
				xSemaphoreGive(xUartSemaphore);

				int8_t len_to_write = p_data_to_send->data_len < 64? p_data_to_send->data_len : 64;
				uint16_t end_of_loop = p_data_to_send->data_len;

				for(uint16_t pos=0; pos<end_of_loop; ){
					int8_t len = uart_tx_chars(EX_UART_NUM, (const char*)&(p_data_to_send->buffer[pos]), len_to_write);//uart_tx_chars

					pos += len;
					len_to_write = (p_data_to_send->data_len - pos) < len_to_write ? p_data_to_send->data_len - pos : len_to_write;
					if(len < len_to_write){
						vTaskDelay(20/portTICK_PERIOD_MS);
					}
					ESP_ERROR_CHECK(esp_task_wdt_reset());
				}

				for(uint8_t i=0; i<5; i++){
					if(!rx_queue_pop_front(&RxQueue)){ // Delete wrong request from TxQueue
						break;
					}
					else vTaskDelay(1/portTICK_PERIOD_MS);
				}
			}
		}
		portYIELD();
		ESP_ERROR_CHECK(esp_task_wdt_reset());
	}
	ESP_ERROR_CHECK(esp_task_wdt_delete(Uart_parser_task_handle));
	vTaskDelete(NULL);
}

void send_to_tx_queue_task()
{
	http_data_t* p_recived_data = NULL;

	ESP_ERROR_CHECK(esp_task_wdt_add(send_to_tx_queue_task_handle));

	while(1){
		if(uxQueueSpacesAvailable(TxQueue) == 0){
			vTaskDelay(1/portTICK_PERIOD_MS);
			continue;
		}
		p_recived_data = (http_data_t*)malloc(1*sizeof(http_data_t));

		p_recived_data->http_addr_start_pos = 0;
		p_recived_data->post_body_start_pos = 0;
		p_recived_data->recieving_stopped_pos = strlen((char*)uart_rx_buf);
		p_recived_data->request_type = 0;
		p_recived_data->buffer[p_recived_data->recieving_stopped_pos]=0;
		memcpy((char*)p_recived_data->buffer, (char*)uart_rx_buf, p_recived_data->recieving_stopped_pos); // copy data from uart_rx_buf to TxQueue item buffer

		if(xSemaphoreTake(xUartSemaphore, (TickType_t)10)){
			if( xQueueGenericSend( TxQueue, ( void * ) &p_recived_data, ( TickType_t ) 10, queueSEND_TO_BACK ) != pdPASS )
			{
				ESP_LOGI("UART", "p_recived_data do not added to TxQueue");
			}
			xSemaphoreGive(xUartSemaphore);
		}
		else ESP_LOGI("UART", "Can't take a semaphore");
		if(Uart_parser_task_handle == NULL){
			xTaskCreate(parserTask, "uart_parser_task", 2048, NULL, 5, Uart_parser_task_handle);
		}
		else{
			vTaskResume(Uart_parser_task_handle);
		}
		if(!urxlen) {
			break;
		}
		ESP_ERROR_CHECK(esp_task_wdt_reset());
	}
	ESP_ERROR_CHECK(esp_task_wdt_delete(send_to_tx_queue_task_handle));
	vTaskDelete(NULL);
}

