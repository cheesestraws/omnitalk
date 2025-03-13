#pragma once

#include "lap/lap.h"
#include "mem/buffers.h"

// returns true if it's been sent, or false if the buffer is still Your Problem
bool ddp_send_via(buffer_t *packet, uint8_t src_socket, 
	uint16_t dest_net, uint8_t dest_node, uint8_t dest_socket,
	uint8_t ddp_type, lap_t* via);

bool ddp_send(buffer_t *packet, uint8_t src_socket, 
	uint16_t dest_net, uint8_t dest_node, uint8_t dest_socket,
	uint8_t ddp_type);
