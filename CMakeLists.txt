cmake_minimum_required(VERSION 3.20)
set(VERSION "0.0.1")
project(CNCpp VERSION ${VERSION} LANGUAGES CXX)

if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
  add_compile_options(-fPIC)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# add_definitions(-DJSON_USE_GLOBAL_UDLS=0 -DJSON_DIAGNOSTICS=0)
#   ____       _
#  / ___|  ___| |_  __   ____ _ _ __ ___
#  \___ \ / _ \ __| \ \ / / _` | '__/ __|
#   ___) |  __/ |_   \ V / (_| | |  \__ \
#  |____/ \___|\__|   \_/ \__,_|_|  |___/

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(BUILD_FLAG "RELEASE_BUILD")
else()
  set(BUILD_FLAG "DEBUG_BUILD")
  add_compile_definitions(DEBUG=1)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(MAIN_DIR ${SRC_DIR}/main)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/usr CACHE PATH "Install path prefix" FORCE)

message(STATUS "SRC_DIR: ${SRC_DIR}")
message(STATUS "MAIN_DIR: ${MAIN_DIR}")
message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
file(GLOB SRC_FILES ${SRC_DIR}/*.cpp)

# Get MADS prefix
execute_process(
  COMMAND mads -p
  OUTPUT_VARIABLE MADS_PREFIX_RAW
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
file(TO_CMAKE_PATH "${MADS_PREFIX_RAW}" MADS_PREFIX)
include_directories(${MADS_PREFIX}/include)
link_directories(${MADS_PREFIX}/lib)
set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${MADS_PREFIX})

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.30")
  # No warning on FetchContent_Populate
  cmake_policy(SET CMP0169 OLD)
endif()

# option(MADS_INSTALL_AGENT "Install the FMU agent" OFF)
# option(MADS_BUILD_FMU "Build and instal the sample FMUs" OFF)

message(STATUS "MADS_PREFIX: ${MADS_PREFIX}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}") 
message(STATUS "CNCpp agent will be installed: ${MADS_INSTALL_AGENT}")


#   _____         _   ____            _
#  |___ / _ __ __| | |  _ \ __ _ _ __| |_ _   _
#    |_ \| '__/ _` | | |_) / _` | '__| __| | | |
#   ___) | | | (_| | |  __/ (_| | |  | |_| |_| |
#  |____/|_|  \__,_| |_|   \__,_|_|   \__|\__, |
#                                         |___/

include(FetchContent)

message(STATUS "Fetching dependencies...")

# FetchContent_Declare(json
#   GIT_REPOSITORY https://github.com/nlohmann/json.git
#   GIT_TAG        v3.12.0
#   GIT_SHALLOW    TRUE
#   EXCLUDE_FROM_ALL
# )


FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        11.0.2
  GIT_SHALLOW    TRUE
)

FetchContent_Declare(rang
  GIT_REPOSITORY https://github.com/agauniyal/rang.git
  GIT_TAG        v3.2
  GIT_SHALLOW    TRUE
)

FetchContent_Declare(yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_SHALLOW    TRUE
  GIT_TAG HEAD
)

FetchContent_Declare(keystroker
  GIT_REPOSITORY https://github.com/pbosetti/keystroker.git
  GIT_SHALLOW    TRUE
  GIT_TAG HEAD
)

set(DOCUMENTATION OFF CACHE BOOL "")
set(INC_BRIDGE_SUPPORT OFF CACHE BOOL "")
set(WITH_APPS OFF CACHE BOOL "")
set(WITH_CLIENTS OFF CACHE BOOL "")
set(WITH_PIC ON CACHE BOOL "")
set(WITH_STATIC_LIBRARIES ON CACHE BOOL "")
set(WITH_TLS OFF CACHE BOOL "")
set(WITH_LIB_CPP ON CACHE BOOL "")

FetchContent_MakeAvailable(fmt rang yaml-cpp keystroker)

message(STATUS "Dependencies fetched.")

include_directories(
  ${fmt_SOURCE_DIR}/include
  ${rang_SOURCE_DIR}/include
  # ${json_SOURCE_DIR}/include
  ${yaml-cpp_SOURCE_DIR}/include
  ${keystroker_SOURCE_DIR}/include
  # /opt/homebrew/include
)


#    ____             __ _                          _    __ _ _
#   / ___|___  _ __  / _(_) __ _ _   _ _ __ ___  __| |  / _(_) | ___  ___
#  | |   / _ \| '_ \| |_| |/ _` | | | | '__/ _ \/ _` | | |_| | |/ _ \/ __|
#  | |__| (_) | | | |  _| | (_| | |_| | | |  __/ (_| | |  _| | |  __/\__ \
#   \____\___/|_| |_|_| |_|\__, |\__,_|_|  \___|\__,_| |_| |_|_|\___||___/
#                          |___/

configure_file(
  ${SRC_DIR}/defines.hpp.in
  ${SRC_DIR}/defines.hpp
)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/examples)

#   _____                    _
#  |_   _|_ _ _ __ __ _  ___| |_ ___
#    | |/ _` | '__/ _` |/ _ \ __/ __|
#    | | (_| | | | (_| |  __/ |_\__ \
#    |_|\__,_|_|  \__, |\___|\__|___/
#                 |___/

# main Library
file(GLOB SRC_FILES ${SRC_DIR}/*.cpp)
file(GLOB HEADERS ${SRC_DIR}/*.hpp)
list(FILTER SRC_FILES EXCLUDE REGEX ".*fsm.*")

# creating our library for source files
add_library(cncpp_lib STATIC ${SRC_FILES})
# target_compile_definitions(cncpp)
target_link_libraries(cncpp_lib PRIVATE fmt::fmt yaml-cpp::yaml-cpp keystroker)
set_target_properties(cncpp_lib PROPERTIES PUBLIC_HEADER "${HEADERS}")
list(APPEND TARGET_LIST cncpp_lib) # this target will be installed

file(GLOB LIB_SOURCES "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp")
add_subdirectory(${SRC_DIR}/main)

# add_executable(point_test ${SRC_DIR}/point.cpp)
# target_compile_definitions(point_test PRIVATE POINT_MAIN)
# target_link_libraries(point_test PRIVATE cncpp_lib)

# add_executable(block_test ${SRC_DIR}/point.cpp ${SRC_DIR}/block.cpp ${SRC_DIR}/machine.cpp)
# target_compile_definitions(block_test PRIVATE BLOCK_MAIN)
# target_link_libraries(block_test PRIVATE cncpp_lib)

# add_executable(blockTRC_test ${SRC_DIR}/point.cpp ${SRC_DIR}/blockTRC.cpp ${SRC_DIR}/machine.cpp)
# target_compile_definitions(blockTRC_test PRIVATE BLOCKTRC_MAIN)
# target_link_libraries(blockTRC_test PRIVATE cncpp_lib)

# add_executable(machine_test ${SRC_DIR}/machine.cpp)
# target_compile_definitions(machine_test PRIVATE MACHINE_MAIN)
# target_link_libraries(machine_test PRIVATE cncpp_lib)

# add_executable(program_test ${SRC_DIR}/program.cpp)
# target_compile_definitions(program_test PRIVATE PROGRAM_MAIN)
# target_link_libraries(program_test PRIVATE cncpp_lib)

# add_executable(simulate ${MAIN_DIR}/simulate.cpp)
# target_link_libraries(simulate PRIVATE cncpp_lib)

# add_executable(cncpp ${MAIN_DIR}/cncpp.cpp)
# target_link_libraries(cncpp PRIVATE cncpp_lib)
# list(APPEND TARGET_LIST cncpp)

# list(APPEND TARGET_LIST simulate)
#   ___           _        _ _
#  |_ _|_ __  ___| |_ __ _| | |
#   | || '_ \/ __| __/ _` | | |
#   | || | | \__ \ || (_| | | |
#  |___|_| |_|___/\__\__,_|_|_|
#

install(TARGETS ${TARGET_LIST}
  BUNDLE DESTINATION bin
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include
)