cmake_minimum_required(VERSION 3.16)

project(AnDrop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(APPLE)
    foreach(qt5_prefix "/opt/homebrew/opt/qt@5" "/usr/local/opt/qt@5")
        if(EXISTS "${qt5_prefix}/lib/cmake/Qt5")
            list(PREPEND CMAKE_PREFIX_PATH "${qt5_prefix}")
        endif()
    endforeach()
endif()

find_package(Qt5 REQUIRED COMPONENTS Widgets Network Test Concurrent)

add_library(androp_core STATIC
    src/protocol/ProtocolCodec.cpp
)

target_include_directories(androp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(androp_core PUBLIC
    Qt5::Core
)

add_executable(AnDrop
    src/app/main.cpp
)

target_link_libraries(AnDrop PRIVATE
    androp_core
    Qt5::Widgets
    Qt5::Network
    Qt5::Concurrent
)

enable_testing()
add_subdirectory(tests)
