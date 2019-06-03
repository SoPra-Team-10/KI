cmake_minimum_required(VERSION 3.10)

# Build options
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Werror -Wno-implicit-fallthrough -march=native -mtune=native")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
endif ()

# Building
project(KI VERSION 0.0.1 DESCRIPTION "KI for SoPra")

set(SOURCES
        ${CMAKE_SOURCE_DIR}/src/Util/Logging.cpp
        ${CMAKE_SOURCE_DIR}/src/Util/Timer.cpp
        ${CMAKE_SOURCE_DIR}/src/Util/ArgumentParser.cpp
        ${CMAKE_SOURCE_DIR}/src/Communication/MessageHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/Communication/Communicator.cpp
        ${CMAKE_SOURCE_DIR}/src/Game/Game.cpp)

set(LIBS pthread stdc++fs SopraGameLogic SopraMessages SopraNetwork)

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable(${PROJECT_NAME} src/main.cpp ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBS})

add_subdirectory(Tests)
