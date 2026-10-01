add_library(RingPassSADX SHARED
    src/plugin.cpp
)

target_link_libraries(RingPassSADX PRIVATE ringpass_protocol)
target_compile_definitions(RingPassSADX PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

set_target_properties(RingPassSADX PROPERTIES
    OUTPUT_NAME "RingPassSADX"
)
