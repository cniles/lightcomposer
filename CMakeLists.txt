cmake_minimum_required(VERSION 3.0.0)
project(lightcomposer VERSION 0.0.0)

include(CTest)
enable_testing()

#set(CMAKE_CXX_STANDARD 11)

include_directories(include)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})



file(GLOB SOURCES "src/*.cpp")

add_executable(lightcompose ${SOURCES})
target_link_libraries(lightcompose avformat avutil avcodec fftw3 z m ${SDL2_LIBRARIES} wiringPi)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)
