#pragma once

#include <stddef.h>

#include <esp_err.h>
#include <esp_eth.h>
#include <esp_netif_types.h>


/* ethernet_output.{c,h} contains a transmit function that maintains
   interface counters and should be used everywhere, and a bit of evil
   to con lwip into using this transmit function rather than the ESP-IDF
   built-in one. */

// send_ethernet is a wrapper around eth_transmit which maintains an
// ifOutOctets
esp_err_t send_ethernet(esp_eth_handle_t hdl, void *buf, size_t length);

void munge_ethernet_output_path(esp_eth_handle_t eth_driver, esp_netif_t *esp_netif);