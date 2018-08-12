cmake_minimum_required(VERSION 3.0.0)
project(lightcomposer VERSION 0.0.0)

include(CTest)
enable_testing()

#set(CMAKE_CXX_STANDARD 11)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_executable(lightcompose ${SOURCES})
target_link_libraries(lightcompose avformat avutil avcodec fftw3 z m SDL SDL_gfx)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)
