#include "net/mdns.h"

#include <esp_err.h>
#include <esp_mac.h>
#include <mdns.h>

#include "net/common.h"

void start_mdns(void) {
	char *hostname = generate_hostname();
	
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(hostname));
    ESP_ERROR_CHECK(mdns_instance_name_set(hostname));
    
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
    
    mdns_txt_item_t serviceTxtData[1] = {
        {"path", "/metrics"},
    };
    
	ESP_ERROR_CHECK(mdns_service_add(NULL, "_prometheus", "_tcp", 80, serviceTxtData, 1));
	
	free(hostname);
}