#include "net/net.h"

#include "lap/llap/llap.h"
#include "lap/sink/sink.h"
#include "lap/registry.h"
#include "net/b2udptunnel/b2udptunnel.h"
#include "net/ethernet/ethernet.h"
#include "net/ltoudp/ltoudp.h"
#include "net/tashtalk/tashtalk.h"
#include "net/common.h"
#include "net/mdns.h"
#include "global_state.h"

void start_net(runloop_info_t* controlplane, runloop_info_t* dataplane) {
	start_common();

	start_ethernet();
	start_mdns();
	start_tashtalk();
	start_ltoudp();
	start_b2udptunnel();
	
	// forcibly clean up after any lingering unit tests
	lap_lsend_mock = NULL;
	
	global_lap_registry = lap_registry_new();
	
	start_sink("SINK-eth", ethertalkv2_get_transport());
//	start_sink("SINK-tt", tashtalk_get_transport());
//	start_sink("SINK-ltoudp", ltoudp_get_transport());
	start_sink("SINK-b2", b2_get_transport());
	
	start_llap("localtalk", tashtalk_get_transport(), global_lap_registry, controlplane, dataplane);
	start_llap("ltoudp", ltoudp_get_transport(), global_lap_registry, controlplane, dataplane);
}
