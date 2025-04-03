set(srcs
	"app/aep/aep.c"
	"app/nbp/nbp.c"
	"app/rtmp/rtmp.c"
	"app/sip/sip.c"
	"app/zip/zip_get_zone_list.c"
	"app/zip/zip_get_network_info.c"
	"app/zip/zip_get_network_info_test.c"
	"app/zip/zip.c"
	"app/zip/zip_test.c"
	"app/app.c"

	"lap/llap/llap.c"
	"lap/llap/llap_test.c"
	"lap/sink/sink.c"
	"lap/id.c"
	"lap/lap.c"
	"lap/lap_test.c"
	"lap/registry.c"
	"lap/registry_test.c"

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
	
	"proto/atp.c"
	"proto/atp_test.c"
	"proto/ddp_test.c"
	"proto/nbp.c"
	"proto/nbp_test.c"
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
	"util/macroman_to_utf8_tables.c"
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

execute_process(COMMAND date "+%Y-%m-%d %H:%M %Z"
                OUTPUT_VARIABLE BUILD_TIMESTAMP
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
add_definitions("-DBUILD_TIMESTAMP=\"${BUILD_TIMESTAMP}\"")

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
