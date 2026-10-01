add_executable(RingPassMonitor
    main.cpp
)

target_link_libraries(RingPassMonitor PRIVATE ringpass_protocol)
target_compile_definitions(RingPassMonitor PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

add_custom_command(TARGET RingPassMonitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_BINARY_DIR}/package/tools"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:RingPassMonitor>"
        "${CMAKE_BINARY_DIR}/package/tools/RingPassMonitor.exe"
)
