#include "web/web.h"

#include <esp_http_server.h>

#include "web/stats.h"

static const char* TAG = "HTTP";

// A basic handler for /
esp_err_t http_root_handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/plain");

	httpd_resp_send_chunk(req, "metrics are at /metrics", HTTPD_RESP_USE_STRLEN);
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

httpd_uri_t http_root = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = http_root_handler,
	.user_ctx = NULL
};

httpd_uri_t http_metrics = {
	.uri = "/metrics",
	.method = HTTP_GET,
	.handler = http_metrics_handler,
	.user_ctx = NULL
};


// Start the httpd
httpd_handle_t start_httpd(void) {
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	httpd_handle_t server = NULL;

	httpd_start(&server, &config);
	
	if(server != NULL) {
		httpd_register_uri_handler(server, &http_root);
		httpd_register_uri_handler(server, &http_metrics);
	}
	return server;
}

void start_web(void) {
	start_httpd();
}
