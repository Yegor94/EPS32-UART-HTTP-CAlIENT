/*
 * http_clint.c
 *
 *  Created on: 26 ма€ 2021 г.
 *      Author: ≈гор
 */


#include "http_client/http_client.h"


static const char *TAG = "HTTP_CLIENT";
SemaphoreHandle_t xHttpSemaphore = NULL;


static void send_post_request(http_data_t* p_request)
{
	char tmp_buf[128];
	char* URL = (char*)malloc(p_request->recieving_stopped_pos - p_request->http_addr_start_pos);
	strncpy(URL, (char*)&(p_request->buffer[p_request->http_addr_start_pos]),(p_request->post_body_start_pos-p_request->http_addr_start_pos-1));
	URL[p_request->post_body_start_pos-p_request->http_addr_start_pos-1] = '\0';

	char *post_data = (char*)&(p_request->buffer[p_request->post_body_start_pos]);

	esp_http_client_config_t config = {
			.url = URL,
			.user_data = (void*)post_data,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Content-Type", "application/json");

	esp_err_t err = esp_http_client_perform(client);
	if (err != ESP_OK) {
		char error[64] = {0};
		sprintf(error, "Failed to perform HTTP client: %s\n", esp_err_to_name(err));
		send_to_uart(error);
	}
	err = esp_http_client_open(client, strlen(post_data));
	if (err != ESP_OK) {
			char error[64] = {0};
			sprintf(error, "Failed to open HTTP connection: %s\n", esp_err_to_name(err));
			send_to_uart(error);
	} else {
		int wlen = esp_http_client_write(client, post_data, strlen(post_data));

		if (wlen < 0) {
			send_to_uart("POST request: write failed");
		}

		int data_read = 128,
				pos = 0;
		char tmp_buf_for_uart[BUF_SIZE] = {0};
		while(data_read){
			data_read = esp_http_client_read_response(client, tmp_buf, 128);
			if (data_read < 0) send_to_uart("Failed to read response");
			else if(data_read == 0){
				send_to_uart(tmp_buf_for_uart);
				break;
			}
			else{
				memcpy((char*)&(tmp_buf_for_uart[pos]), tmp_buf, data_read);
				pos+=data_read;
				if(pos >= BUF_SIZE){ // protect tmp_buf_for_uart from out of range if http response length more then BUF_SIZE
					pos=0;
					send_to_uart(tmp_buf_for_uart);
				}
			}
		}
	}
	free(URL);

	esp_http_client_cleanup(client);

	for(uint8_t i=0; i<5; i++){
		if(!tx_queue_pop_front(&TxQueue)){ // Delete wrong request from TxQueue
			break;
		}
		else vTaskDelay(1/portTICK_PERIOD_MS);
	}
}

static void send_get_request(http_data_t* p_request)
{
	char* URL = (char*)malloc(p_request->recieving_stopped_pos - p_request->http_addr_start_pos);
	memcpy(URL, (char*)&(p_request->buffer[p_request->http_addr_start_pos]), p_request->recieving_stopped_pos - p_request->http_addr_start_pos+1);

	esp_http_client_config_t config = {
			.url = URL,
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_http_client_set_method(client, HTTP_METHOD_GET);
	esp_err_t err = esp_http_client_open(client, 0);
	if (err != ESP_OK) {
		char error[64] = {0};
		sprintf(error, "Failed to open HTTP connection: %s\n", esp_err_to_name(err));
		send_to_uart(error);
	} else {
		int headers_len = esp_http_client_fetch_headers(client);
		if (headers_len < 0) {
			send_to_uart("HTTP client fetch headers failed\n");
		} else {
			int data_read = 128,
					pos = 0;
			char tmp_buf[1024];
			char tmp_buf_for_uart[BUF_SIZE] = {0};
			while(data_read){
				data_read = esp_http_client_read_response(client, tmp_buf, 512);
				if (data_read < 0) send_to_uart("Failed to read response");
				else if(data_read == 0){
					tmp_buf_for_uart[pos] = '\0';
					send_to_uart(tmp_buf_for_uart);
					break;
				}
				else{
					memcpy((char*)&(tmp_buf_for_uart[pos]), tmp_buf, data_read);
					pos+=data_read;
					if(pos >= BUF_SIZE){ // protect tmp_buf_for_uart from out of range if http response length more then BUF_SIZE
						tmp_buf_for_uart[pos-data_read] = '\0';
						pos=0;
						send_to_uart(tmp_buf_for_uart);
					}
				}
			}
		}
	}
	esp_http_client_close(client);
	free(URL);

	for(uint8_t i=0; i<5; i++){
		if(!tx_queue_pop_front(&TxQueue)){ // Delete wrong request from TxQueue
			break;
		}
		else vTaskDelay(1/portTICK_PERIOD_MS);
	}
}

void http_client_task(void *pvParameters)
{
	xHttpSemaphore = xSemaphoreCreateMutex();
	if(xHttpSemaphore == NULL) ESP_LOGI(TAG, "xParserSemaphore do not created");
	http_data_t* p_data_to_send = NULL;

	ESP_ERROR_CHECK(esp_task_wdt_add(http_client_task_handle));

	while(1){
		if(uxQueueSpacesAvailable(TxQueue) == TX_QUEUE_SIZE){
			break;
		}
		if(xSemaphoreTake(xHttpSemaphore, (TickType_t)10) == pdTRUE &&
				xQueueGenericReceive(TxQueue, &(p_data_to_send), (TickType_t)10, pdTRUE))
		{
			xSemaphoreGive(xHttpSemaphore);
			if(!p_data_to_send->http_addr_start_pos) continue;

			switch(p_data_to_send->request_type){

				case GET: {send_get_request(p_data_to_send); break;}

				case POST: {send_post_request(p_data_to_send); break;}

				default: send_to_uart("Wrong http request type.\n");
			}
		}
		else ESP_LOGE(TAG, "Can't take a semaphore");
		ESP_ERROR_CHECK(esp_task_wdt_reset());
	}
	ESP_ERROR_CHECK(esp_task_wdt_delete(http_client_task_handle));
	vTaskDelete(NULL);
}

