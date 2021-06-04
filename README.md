Platform: ESP32
Framework: ESP-IDF

Description:
This application is designed to send messages received via UART 
to an HTTP server and broadcast the response from the HTTP server 
to UART.

Protocol for sending message:
http <REQ> <URL> <BODY>
REQ - GET or POST request.
URL - remote server address. For example http://httpbin.org
BODY - request body in case of POST.

Exemples of request:
http GET http://httpbin.org/get
http POST http://httpbin.org/post {"value" : "123456"}
Note: between every nenber of request must be onle 1 character ' ' (space).
For keywords, upper and lower case characters matter.
For keyword 'http' all characters must be in lower case
For keywords 'GET' and 'POST' all characters must be in upper case
For http address and POST body upper and lower case characters
does not matter.
