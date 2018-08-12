cmake_minimum_required(VERSION 3.0.0)
project(HelloWorld VERSION 0.0.0)

include(CTest)
enable_testing()

#set(CMAKE_CXX_STANDARD 11)

add_executable(HelloWorld main.cpp)
target_link_libraries(HelloWorld avformat avutil avcodec fftw3 z m SDL SDL_gfx)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)
