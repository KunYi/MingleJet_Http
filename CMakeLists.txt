CMAKE_MINIMUM_REQUIRED(VERSION 3.15.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /your/msvc/flags")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /your/msvc/flags")
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    # extra warning and convert warning to error level
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -pedantic-errors")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -o2")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -ggdb3")
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries (.dll/.so)" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries (.lib/.a)" FORCE)
set(LIBUV_BUILD_SHARED OFF CACHE BOOL "Build libuv as a static library" FORCE)
set(LIBUV_BUILD_TESTS OFF CACHE BOOL "Disable libuv tests" FORCE)
set(LIBUV_BUILD_BENCHMARKS OFF CACHE BOOL "Disable libuv benchmarks" FORCE)
set(LIBUV_BUILD_DOCS OFF CACHE BOOL "Disable libuv documentation" FORCE)

# Add subdirectories for third-party libraries
add_subdirectory(thirdparty/libuv)
add_subdirectory(thirdparty/llhttp)

PROJECT(MingleJet VERSION 0.0.1)

SET(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
SET(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

INCLUDE_DIRECTORIES(
    ${INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libuv/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llhttp/include
)

FILE(GLOB SOURCES "${SOURCE_DIR}/*.c")

ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCES})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} uv_a llhttp_static)

ADD_CUSTOM_TARGET(memchk
    COMMAND ${CMAKE_COMMAND} -E echo "Running Valgrind..."
    COMMAND valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind.log -s ${CMAKE_BINARY_DIR}/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    COMMENT "Running Valgrind on the executable"
)
