set(COMPONENT_SRCS CLI.c cmd_system.c cmd_TLS3001.c ../pattern_generator.c cmd_E131.c cmd_wifi.c)
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_REQUIRES fatfs nvs_flash console freertos)

register_component()
