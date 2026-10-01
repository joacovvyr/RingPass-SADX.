add_library(RingPassER SHARED
    src/dllmain.cpp
)

target_link_libraries(RingPassER PRIVATE ringpass_protocol)
target_compile_definitions(RingPassER PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

set_target_properties(RingPassER PROPERTIES
    OUTPUT_NAME "RingPassER"
)
