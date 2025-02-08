set(srcs
	"lap/sink/sink.c"

	"mem/buffers.c"

	"net/ethernet/ethernet.c"
	"net/ethernet/ethernet_output.c"
	"net/tashtalk/state_machine.c"
	"net/tashtalk/tashtalk.c"
	"net/tashtalk/uart.c"
	"net/common.c"
	"net/mdns.c"
	"net/net.c"
	"net/transport.c"
	
	"util/crc.c"
	"util/crc32.c"

	"web/stats.c"
	"web/stats_memory.c"
	"web/util.c"
	"web/web.c"

	"controlplane_runloop.c"
	"router_runloop.c"
	"main.c"
)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS ".")
