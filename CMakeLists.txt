cmake_minimum_required(VERSION 3.16)
project(tcp_file_transfer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_library(transfer_core
    src/crypto/Sha256.cpp
    src/network/TcpSocket.cpp
    src/protocol/Protocol.cpp
    src/transfer/FileReader.cpp
    src/transfer/FileWriter.cpp
)

target_include_directories(transfer_core PUBLIC include)
target_compile_definitions(transfer_core PRIVATE $<$<CONFIG:Debug>:TCPFT_DEBUG>)

if(WIN32)
    target_link_libraries(transfer_core PUBLIC ws2_32)
endif()

add_executable(ft-server src/server/main.cpp)
target_link_libraries(ft-server PRIVATE transfer_core)

add_executable(ft-client src/client/main.cpp)
target_link_libraries(ft-client PRIVATE transfer_core)

include(GNUInstallDirs)

install(
    TARGETS ft-server ft-client
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

enable_testing()
add_subdirectory(tests)
