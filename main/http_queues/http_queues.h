/*
 * http_queues.h
 *
 *  Created on: 26 ма€ 2021 г.
 *      Author: ≈гор
 */

#ifndef MAIN_HTTP_QUEUES_HTTP_QUEUES_H_
#define MAIN_HTTP_QUEUES_HTTP_QUEUES_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"


#define BUFFER_SIZE (4096)

typedef struct{
	uint8_t request_type;           /*!< Type of request to http server: 0 - no request type, 1 - GET, 2 - POST*/
	uint16_t recieving_stopped_pos; /*!< pointer to last element which was written to array 'buffer' when receive data from UART */
	uint16_t http_addr_start_pos;     /*!< Position to first element of URL at request*/
	uint16_t post_body_start_pos;     /*!< Position to first element of POST BODY if http request method !POST p_post_body_start == NULL*/
	uint8_t buffer[BUFFER_SIZE];    /*!< Buffer to place received data or data for transmit*/
}http_data_t;

typedef struct{
	int16_t data_len;              /*!< Length of data placed in 'buffer'*/
	uint8_t buffer[BUFFER_SIZE];    /*!< Buffer to place data which will be sent to UART*/
}data_for_uart_tx_t;


#define TX_ITEM_SIZE sizeof( http_data_t* ) /*!< Size of item for TX queue*/
#define TX_QUEUE_SIZE (15) /*!< Size of queue for request messages from user to http server*/

#define RX_ITEM_SIZE sizeof( data_for_uart_tx_t* ) /*!< Size of item for RX queue*/
#define RX_QUEUE_SIZE (30) /*!< Size of queue for response messages from http server and error messages dedicated for user */

/*
 * RxQueue - queue for response messages from http server and error messages dedicated for user
 * TxQueue - queue for request messages from user to http server
 */
extern QueueHandle_t RxQueue,
					 TxQueue;

extern TaskHandle_t Uart_parser_task_handle,
		uart_tx_task_handle,
		http_client_task_handle,
		send_to_tx_queue_task_handle,
		http_client_task_handle;

/*
 * uint8_t initQueues(void) - create RxQueue and TxQueue which contains a pointers to structure http_data_t
 * Number of element of queues equal TX_QUEUE_SIZE and RX_QUEUE_SIZE
 *
 * Returned value:
 * '0' - success
 * '-1' - error of creating queues
 *
 * */
uint8_t initQueues(void);

/*
 * tx_queue_pop_front(QueueHandle_t* p_queue_handle) - delete first
 * element in Tx Queue
 *
 * note: If you pass as an argument a pointer to a queue of a type other
 * than http_data_t, the results of deletion will be unexpected.
 * The function is safe for use in threads.
 *
 * param: *
 * @p_queue_handle - pointer to TxQueue
 *
 * returned value:
 * 0 - successful removal item
 * -1 - error occurred. Item was not deleted
 *
 * */
uint8_t tx_queue_pop_front(QueueHandle_t* p_queue_handle);

/*
 * rx_queue_pop_front(QueueHandle_t* p_queue_handle) - delete first
 * element in Rx Queue
 *
 * note: If you pass as an argument a pointer to a queue of a type other
 * than data_for_uart_tx_t, the results of deletion will be unexpected.
 * The function is safe for use in threads.
 *
 * param: *
 * @p_queue_handle - pointer to RxQueue
 *
 * returned value:
 * 0 - successful removal item
 * -1 - error occurred. Item was not deleted
 *
 * */
uint8_t rx_queue_pop_front(QueueHandle_t* p_queue_handle);

#endif /* MAIN_HTTP_QUEUES_HTTP_QUEUES_H_ */
