cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(libhttp-learning)

find_package(fmt REQUIRED)

add_subdirectory(libhttp)

add_executable(runclient
 runclient.cpp
)
add_executable(runserver
 runserver.cpp
)
target_link_libraries(runserver
 PRIVATE
  libhttp
  fmt
)
target_link_libraries(runclient
 PRIVATE
  libhttp
  fmt
)
