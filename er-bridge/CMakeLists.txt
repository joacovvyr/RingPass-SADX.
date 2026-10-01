if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "RingPassER must be built as x64.")
endif()

add_library(RingPassER SHARED
    src/dllmain.cpp
    src/enemy_reader.cpp
    src/host_environment.cpp
    src/ground_probe_service.cpp
    src/module_scanner.cpp
    src/stable_frame_service.cpp
    src/world_reader.cpp
)

target_link_libraries(RingPassER PRIVATE ringpass_protocol)
target_compile_definitions(RingPassER PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

set_target_properties(RingPassER PROPERTIES
    OUTPUT_NAME "RingPassER"
    PREFIX ""
)

add_custom_command(TARGET RingPassER POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_BINARY_DIR}/package/elden-ring"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:RingPassER>"
        "${CMAKE_BINARY_DIR}/package/elden-ring/RingPassER.dll"
)

target_link_libraries(RingPassER PRIVATE Version)
