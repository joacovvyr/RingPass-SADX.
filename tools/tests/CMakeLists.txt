add_executable(RingPassProtocolTests protocol_tests.cpp)
target_link_libraries(RingPassProtocolTests PRIVATE ringpass_protocol)
target_compile_definitions(RingPassProtocolTests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

add_test(NAME RingPassProtocolTests COMMAND RingPassProtocolTests)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_executable(RingPassScannerTests scanner_tests.cpp
        ../../er-bridge/src/module_scanner.cpp)
    target_include_directories(RingPassScannerTests PRIVATE ../../er-bridge/src)
    target_compile_definitions(RingPassScannerTests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    add_test(NAME RingPassScannerTests COMMAND RingPassScannerTests)
endif()
