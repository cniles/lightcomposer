cmake_minimum_required(VERSION 3.0.0)
project(lightcomposer VERSION 0.0.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)

include(CTest)
enable_testing()

#set(CMAKE_CXX_STANDARD 11)

include_directories(include)

find_package(SDL2 REQUIRED)
include_directories(lightcomposer ${SDL2_INCLUDE_DIRS})

find_package(SDL2_gfx REQUIRED)
include_directories(lightcomposer ${SDL2_GFX_INCLUDE_DIRS})

find_package(Boost REQUIRED)
include_directories(lightcomposer ${BOOST_INCLUDE_DIRS})

find_package(LIBAV REQUIRED)
include_directories(lightcomposer ${LIBAV_INCLUDE_DIRS})

find_package(FFTW REQUIRED)
include_directories(lightcomposer ${FFTW_INCLUDE_DIRS})

find_library(WIRINGPI_LIBRARY wiringPi)

if (WIRINGPI_LIBRARY STREQUAL "WIRINGPI_LIBRARY-NOTFOUND")
    set(WIRINGPI_LIBRARY "")
else()
    add_definitions(-DWIRINGPI)
endif()

file(GLOB SOURCES "src/*.cpp")

add_executable(lightcompose ${SOURCES})

target_link_libraries(lightcompose ${LIBAV_LIBRARIES} ${FFTW_LIBRARIES} z m ${SDL2_LIBRARIES} ${SDL2_GFX_LIBRARIES} ${WIRINGPI_LIBRARY})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)
