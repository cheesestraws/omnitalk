#include "net/net.h"

#include "net/ethernet.h"
#include "net/mdns.h"

void start_net(void) {
	start_ethernet();
	start_mdns();
}