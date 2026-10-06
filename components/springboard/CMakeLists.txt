get_filename_component(S3_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../firmware" ABSOLUTE)
file(GLOB APP_SOURCES "${S3_ROOT}/src/apps/app_*.c")
file(GLOB OS_SOURCES "${S3_ROOT}/src/os/*.c")
file(GLOB NATIVE_FONTS "${CMAKE_CURRENT_LIST_DIR}/../../src/port/fonts/*.c")
set(DOOM_NAMES dummy am_map doomdef doomstat dstrings d_event d_items d_iwad d_loop d_main d_mode d_net f_finale f_wipe g_game
 hu_lib hu_stuff info i_cdmus i_endoom i_joystick i_scale i_sound i_system i_timer memio m_argv m_bbox m_cheat m_config
 m_controls m_fixed m_menu m_misc m_random p_ceilng p_doors p_enemy p_floor p_inter p_lights p_map p_maputl p_mobj p_plats
 p_pspr p_saveg p_setup p_sight p_spec p_switch p_telept p_tick p_user r_bsp r_data r_draw r_main r_plane r_segs r_sky
 r_things sha1 sounds statdump st_lib st_stuff s_sound tables v_video wi_stuff w_checksum w_file w_main w_wad z_zone
 w_file_stdc i_input i_video doomgeneric mus2mid)
set(DOOM_SOURCES "")
set(QJS_SOURCES "")
foreach(n quickjs dtoa libregexp libunicode cutils browser_js)
 list(APPEND QJS_SOURCES "${S3_ROOT}/src/third_party/quickjs/${n}.c")
endforeach()
foreach(n ${DOOM_NAMES})
 list(APPEND DOOM_SOURCES "${S3_ROOT}/src/third_party/doomgeneric/${n}.c")
endforeach()
idf_component_register(SRCS ${APP_SOURCES} ${OS_SOURCES} ${NATIVE_FONTS}
 "${S3_ROOT}/src/apps/apps.c" "${S3_ROOT}/src/apps/media.c" "${S3_ROOT}/src/apps/widgets_extra.c" "${S3_ROOT}/src/mouse_cursor_icon.c"
 "${CMAKE_CURRENT_LIST_DIR}/../../src/port/hardware.c" "${CMAKE_CURRENT_LIST_DIR}/../../src/port/commands.c" "${CMAKE_CURRENT_LIST_DIR}/../../src/port/effects.c"
 "${CMAKE_CURRENT_LIST_DIR}/../../src/port/smoke.c" "${CMAKE_CURRENT_LIST_DIR}/../../src/port/st7735.c" "${CMAKE_CURRENT_LIST_DIR}/../../src/port/s3_wifi.c" "${CMAKE_CURRENT_LIST_DIR}/../../src/port/s3_doom.c" "${CMAKE_CURRENT_LIST_DIR}/../../src/port/s3_vector.c"
 "${S3_ROOT}/src/apps/doom_glue.c" ${DOOM_SOURCES} ${QJS_SOURCES}
 LDFRAGMENTS "doom_memory.lf"
 INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../src/port" "${S3_ROOT}" "${S3_ROOT}/src"
 REQUIRES lvgl nvs_flash esp_http_client esp_netif esp_wifi esp_event vfs pthread mbedtls esp_driver_gpio esp_driver_spi esp_driver_usb_serial_jtag esp_lcd esp_timer esp_psram)
target_compile_definitions(${COMPONENT_LIB} PUBLIC OS_FX_BLUR=1 OS_FX_LENS=1 OS_FX_HEAVY=1 S3_NATIVE_LAYOUT=1)
target_compile_definitions(${COMPONENT_LIB} PRIVATE S3_DOOM DOOMGENERIC_RESX=320 DOOMGENERIC_RESY=200)
target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_VERSION="2025-09-13")
set_source_files_properties(${QJS_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")
target_include_directories(${COMPONENT_LIB} PRIVATE "${S3_ROOT}/src/third_party/doomgeneric")
set_source_files_properties(${DOOM_SOURCES} PROPERTIES COMPILE_OPTIONS "-w;-fno-strict-aliasing;-fwrapv")
set_source_files_properties("${S3_ROOT}/src/third_party/doomgeneric/r_data.c" "${S3_ROOT}/src/third_party/doomgeneric/r_things.c" PROPERTIES COMPILE_OPTIONS "-w;-O1;-fno-strict-aliasing;-fwrapv")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)

set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/../../src/port/hardware.c" PROPERTIES COMPILE_OPTIONS "-O3")
