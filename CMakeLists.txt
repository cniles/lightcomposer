cmake_minimum_required(VERSION 3.0.0)
project(lightcomposer VERSION 0.0.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)

include(CTest)
enable_testing()

#set(CMAKE_CXX_STANDARD 11)

include_directories(include)

find_package(Catch2 QUIET)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(SDL2_gfx)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(LIBAV REQUIRED)
include_directories(${LIBAV_INCLUDE_DIRS})

find_package(FFTW REQUIRED)
include_directories(${FFTW_INCLUDE_DIRS})

find_library(WIRINGPI_LIBRARY wiringPi)

if (WIRINGPI_LIBRARY STREQUAL "WIRINGPI_LIBRARY-NOTFOUND")
    set(WIRINGPI_LIBRARY "")
else()
    add_definitions(-DWIRINGPI)
endif()

if (SDL2_GFX_FOUND)
  add_definitions(-DSDLGFX)
  include_directories(lightcomposer ${SDL2_GFX_INCLUDE_DIRS})
  message("gfx enable")
else()
  set(SDL2_GFX_LIBRARIES "")
  message("gfx disable")
endif()

add_library(packet_queue OBJECT src/packet_queue.cpp)
add_library(audio OBJECT src/audio.cpp)
add_library(draw OBJECT src/draw.cpp)

add_executable(lightcomposer $<TARGET_OBJECTS:audio> $<TARGET_OBJECTS:packet_queue> $<TARGET_OBJECTS:draw> src/main.cpp)

if(APPLE)
  message("Using FLT audio")
  add_definitions(-DFMT_FLT)
else()
  message("Using S16 audio")
  add_definitions(-DFMT_16)
endif()

target_link_libraries(lightcomposer atomic ${LIBAV_LIBRARIES} ${FFTW_LIBRARIES} z m ${SDL2_LIBRARIES} ${SDL2_GFX_LIBRARIES} ${WIRINGPI_LIBRARY})


# Tests
if (TARGET Catch2::Catch2)
add_executable(audio_test test/audio_test.cpp $<TARGET_OBJECTS:audio> $<TARGET_OBJECTS:packet_queue>)
  target_link_libraries(audio_test Catch2::Catch2 ${LIBAV_LIBRARIES} ${SDL2_LIBRARIES})
  include(Catch)
  catch_discover_tests(audio_test)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)
