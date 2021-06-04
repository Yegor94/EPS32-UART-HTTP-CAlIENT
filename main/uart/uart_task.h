/*
 * uart_task.h
 *
 *  Created on: 26 ма€ 2021 г.
 *      Author: ≈гор
 */

#ifndef MAIN_UART_UART_TASK_H_
#define MAIN_UART_UART_TASK_H_

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "uart/uart_task.h"

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
#define EX_UART_NUM UART_NUM_0

#define PATTERN_CHR_NUM    (3)
#define END_OF_LINE_SYMB '\n'

#define UART_TASK_BOUDE_RATE (115200)
#define UART_TASK_DATA_BITS (UART_DATA_8_BITS)
#define UART_TASK_PARITY (UART_PARITY_DISABLE)
#define UART_TASK_STOP_BITS (UART_STOP_BITS_1)
#define UART_TASK_HW_FLOWCTRL (UART_HW_FLOWCTRL_DISABLE)


/*
 * void uart_init() - UART initialization function/
 * Must be called once. Or only after call uart_driver_delete(uart_port_t uart_num)
 */
void uart_init();


/*
 * send_to_tx_queue_task() - for internal usage only.
 * It called from ISR when new line character was received from UART FIFO,
 * create new item in TX queue and copy received by UART string
 * from uart_rx_buf to buffer allocated at last item of TX queue.
 * Then create or continue an parserTask() defined in ./main/http_parser/http_parser.h
 * */
void send_to_tx_queue_task();

/*
 * send_to_uart(char* data) - this function push back
 * data in argument to end of RxQueue by copy. It dynamically
 * allocate memory for new queue item. If queue hasn't free cells for
 * new item it wait 10 ms for free space.
 *
 * Param:
 *
 * @char* data - pointer to string which need add to end of RxQueue
 */
void send_to_uart(char* data);

/*
 * uart_tx_task(void *pvParameters) - take first item from RX queue
 * and send it to UART FIFO. Then delete first item from RX queue and
 * check queue for next item. If RX queue is empty task will delete
 * from task list of FreeRTOS.
 */
void uart_tx_task(void *pvParameters);



#endif /* MAIN_UART_UART_TASK_H_ */
