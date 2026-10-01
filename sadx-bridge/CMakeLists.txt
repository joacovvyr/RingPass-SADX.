set(SADX_MOD_LOADER_ROOT "" CACHE PATH
    "Path to a checkout of X-Hax/sadx-mod-loader")

if(NOT SADX_MOD_LOADER_ROOT)
    message(FATAL_ERROR
        "Set SADX_MOD_LOADER_ROOT to your sadx-mod-loader checkout. "
        "Example: -DSADX_MOD_LOADER_ROOT=C:/dev/sadx-mod-loader")
endif()

set(SADX_MOD_LOADER_INCLUDE
    "${SADX_MOD_LOADER_ROOT}/SADXModLoader/include")

if(NOT EXISTS "${SADX_MOD_LOADER_INCLUDE}/SADXModLoader.h")
    message(FATAL_ERROR
        "SADXModLoader.h was not found at ${SADX_MOD_LOADER_INCLUDE}")
endif()

add_library(RingPassSADX SHARED
    src/plugin.cpp
    src/target_proxy_adapter.cpp
)

target_include_directories(RingPassSADX PRIVATE
    "${SADX_MOD_LOADER_INCLUDE}"
)

target_link_libraries(RingPassSADX PRIVATE ringpass_protocol)
target_compile_definitions(RingPassSADX PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

set_target_properties(RingPassSADX PROPERTIES
    OUTPUT_NAME "RingPassSADX"
    PREFIX ""
)

set(RINGPASS_SADX_PACKAGE_DIR
    "${CMAKE_BINARY_DIR}/package/RingPass-SADX")

add_custom_command(TARGET RingPassSADX POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${RINGPASS_SADX_PACKAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:RingPassSADX>"
        "${RINGPASS_SADX_PACKAGE_DIR}/RingPassSADX.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/mod.ini"
        "${RINGPASS_SADX_PACKAGE_DIR}/mod.ini"
)
