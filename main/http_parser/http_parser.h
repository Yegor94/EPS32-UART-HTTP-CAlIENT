/*
 * http_parser.h
 *
 *  Created on: 26 ма€ 2021 г.
 *      Author: ≈гор
 */

#ifndef MAIN_HTTP_PARSER_HTTP_PARSER_H_
#define MAIN_HTTP_PARSER_HTTP_PARSER_H_

#include "http_queues/http_queues.h"
#include "http_client/http_client.h"

enum RequestType{
	GET = 1,
	POST
};

/*
 * parserTask(void* pvParameters) - take first item in
 * TX queue but do not delete it from queue and check string in buffer
 * for protocol compliance.
 * Then detect http address start position and POST body start position
 * (if request type is POST) in buffer of item assigns the corresponding
 * members of the structure http_data_t. If parsing complete successfully
 * call/resume http_client_task(void *pvParameters) which defined in file
 * ./main/http_client/http_client.h
 */
void parserTask(void* pvParameters);


#endif /* MAIN_HTTP_PARSER_HTTP_PARSER_H_ */
