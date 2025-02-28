set(srcs
	"app/aep/aep.c"
	"app/rtmp/rtmp.c"
	"app/app.c"

	"lap/llap/llap.c"
	"lap/sink/sink.c"
	"lap/id.c"
	"lap/lap.c"

	"mem/buffers.c"

	"net/b2udptunnel/b2udptunnel.c"
	"net/ethernet/ethernet.c"
	"net/ethernet/ethernet_output.c"
	"net/ltoudp/ltoudp.c"
	"net/tashtalk/state_machine.c"
	"net/tashtalk/tashtalk.c"
	"net/tashtalk/uart.c"
	"net/common.c"
	"net/mdns.c"
	"net/net.c"
	"net/transport.c"
	
	"proto/rtmp.c"
	
	"table/routing/table_impl.c"
	
	"util/crc.c"
	"util/crc32.c"

	"web/stats.c"
	"web/stats_memory.c"
	"web/util.c"
	"web/web.c"

	"controlplane_runloop.c"
	"global_state.c"
	"router_runloop.c"
	"runloop.c"
	"main.c"
)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS ".")

execute_process(COMMAND git describe --always --tags --dirty
                OUTPUT_VARIABLE GIT_VERSION
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
add_definitions("-DGIT_VERSION=\"${GIT_VERSION}\"")
