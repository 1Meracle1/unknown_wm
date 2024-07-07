cmake_minimum_required(VERSION 3.28)
project(unknown_wm)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig)
pkg_check_modules(X11 REQUIRED x11)

add_executable(${PROJECT_NAME} 
  src/main.cpp
  src/window_manager.h
  src/window_manager.cpp
  src/config.h
)

set(SPD_LOG_DIR third_party/spdlog)
add_subdirectory(${SPD_LOG_DIR})

target_compile_definitions(${PROJECT_NAME} PRIVATE 
  SRC_DIR="${CMAKE_SOURCE_DIR}"
)

target_include_directories(${PROJECT_NAME} PRIVATE 
  ${X11_INCLUDE_DIRS}
  ${SPD_LOG_DIR}/include
)
target_link_libraries(${PROJECT_NAME} PRIVATE 
  ${X11_LIBRARIES}
  spdlog::spdlog
)
target_compile_options(${PROJECT_NAME} PRIVATE -pedantic -Wall -Wextra 
  ${X11_CFLAGS}
)
target_precompile_headers(${PROJECT_NAME} PRIVATE
  <spdlog/spdlog.h>
)
