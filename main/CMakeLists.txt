# Embed the server root certificate into the final binary
#
# (If this was a component, we would set COMPONENT_EMBED_TXTFILES here.)
idf_component_register(SRCS "main.c"
							"uart/uart_task.c"
							"http_parser/http_parser.c"
							"http_client/http_client.c"
							"http_queues/http_queues.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES howsmyssl_com_root_cert.pem)