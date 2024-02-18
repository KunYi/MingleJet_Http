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

FIND_PACKAGE(libuv REQUIRED)
FIND_PACKAGE(llhttp REQUIRED)

PROJECT(MingleJet VERSION 0.0.1)

SET(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
SET(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

INCLUDE_DIRECTORIES(${INCLUDE_DIR})

FILE(GLOB SOURCES "${SOURCE_DIR}/*.c")

ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCES})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} uv llhttp jansson jwt zlog)

# ADD_CUSTOM_TARGET(memchk
#     COMMAND ${CMAKE_COMMAND} -E echo "Running Valgrind..."
#     COMMAND valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind.log -s ${CMAKE_BINARY_DIR}/${PROJECT_NAME}
#     DEPENDS ${PROJECT_NAME}
#     COMMENT "Running Valgrind on the executable"
# )
