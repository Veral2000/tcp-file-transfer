add_executable(tcpft_tests
    test_file_io.cpp
    test_protocol.cpp
)

target_link_libraries(tcpft_tests PRIVATE transfer_core)
target_include_directories(tcpft_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_test(NAME tcpft_tests COMMAND tcpft_tests)
