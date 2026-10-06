get_filename_component(S3_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../firmware" ABSOLUTE)
file(GLOB_RECURSE LV_SOURCES "${S3_ROOT}/lvgl/src/*.c" "${S3_ROOT}/lvgl/src/*.cpp")
# These mutually exclusive optional GPU backends duplicate a basename, which
# PlatformIO flattens for external sources. Neither backend is enabled here.
list(FILTER LV_SOURCES EXCLUDE REGEX "/(vg_lite_driver|vg_lite_tvg)/")
file(GLOB SIMD_SOURCES "${CMAKE_CURRENT_LIST_DIR}/../../src/port/simd/*_esp32s3.S" "${CMAKE_CURRENT_LIST_DIR}/../../src/port/simd/s3_simd.c" "${CMAKE_CURRENT_LIST_DIR}/../../src/port/simd/s3_thorvg.c")
idf_component_register(SRCS ${LV_SOURCES} ${SIMD_SOURCES}
    INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../src/port" "${CMAKE_CURRENT_LIST_DIR}/../../src/port/simd" "${S3_ROOT}" "${S3_ROOT}/lvgl" "${S3_ROOT}/lvgl/include"
    REQUIRES esp_timer)
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    LV_CONF_PATH="${CMAKE_CURRENT_LIST_DIR}/../../include/lv_conf.h")

# Optimize the measured raster hot paths without changing shell geometry or physics.
file(GLOB_RECURSE S3_RASTER_SOURCES "${S3_ROOT}/lvgl/src/draw/sw/*.c")
set_source_files_properties(${S3_RASTER_SOURCES} PROPERTIES COMPILE_OPTIONS "-O3")
file(GLOB S3_THORVG_SOURCES "${S3_ROOT}/lvgl/src/libs/thorvg/*.cpp")
set_source_files_properties(${S3_THORVG_SOURCES} PROPERTIES COMPILE_OPTIONS "-O3;-Wno-error=stringop-truncation")
