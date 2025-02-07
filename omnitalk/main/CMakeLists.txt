set(srcs
	"net/common.c"
	"net/ethernet.c"
	"net/ethernet_output.c"
	"net/tashtalk/tashtalk.c"
	"net/tashtalk/uart.c"
	"net/mdns.c"
	"net/net.c"

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
