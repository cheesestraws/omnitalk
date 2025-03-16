set(srcs
	"app/aep/aep.c"
	"app/rtmp/rtmp.c"
	"app/zip/zip.c"
	"app/app.c"

	"lap/llap/llap.c"
	"lap/llap/llap_test.c"
	"lap/sink/sink.c"
	"lap/id.c"
	"lap/lap.c"

	"mem/buffers.c"
	"mem/buffers_test.c"

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
	
	"proto/ddp_test.c"
	"proto/rtmp.c"
	"proto/zip.c"
	"proto/zip_test.c"
	
	"table/aarp/table.c"
	"table/aarp/table_test.c"
	"table/routing/table_impl.c"
	"table/routing/table_test.c"
	"table/zip/table_impl.c"
	"table/zip/table_test.c"
	
	"util/event/event.c"
	"util/event/event_test.c"
	"util/crc.c"
	"util/crc32.c"
	"util/pstring.c"
	"util/pstring_test.c"
	"util/string.c"

	"web/stats.c"
	"web/stats_memory.c"
	"web/util.c"
	"web/web.c"

	"controlplane_runloop.c"
	"ddp_send.c"
	"global_state.c"
	"router_runloop.c"
	"runloop.c"
	"test.c"
	"main.c"
)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS ".")

execute_process(COMMAND git describe --always --tags --dirty
                OUTPUT_VARIABLE GIT_VERSION
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
add_definitions("-DGIT_VERSION=\"${GIT_VERSION}\"")

# Generate include files for tests, stats and metadata.
# Technique from https://github.com/espressif/esp-idf/issues/9885 w/ tweaks.

add_custom_command(OUTPUT test.inc
                   COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/mktests.pl)
add_custom_target(tests_file ALL DEPENDS test.inc)

add_custom_command(OUTPUT web/stats.inc
                   COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/web/mkstats.pl)
add_custom_target(stats_file ALL DEPENDS web/stats.inc)

add_custom_command(OUTPUT web/metadata.inc
                   COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/web/mkmetadata.pl)
add_custom_target(metadata_file ALL DEPENDS web/metadata.inc)
