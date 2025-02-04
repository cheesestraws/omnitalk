set(srcs
	"web/stats.c"
	"web/util.c"
	"web/web.c"

	"controlplane_runloop.c"
	"router_runloop.c"
	"main.c"
)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS ".")
