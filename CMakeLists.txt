cmake_minimum_required(VERSION 3.28)
project(unknown_wm)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig)
pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(XCB REQUIRED xcb)
pkg_check_modules(XCB_KEYSYMS REQUIRED xcb-keysyms)
pkg_check_modules(XCB_UTIL REQUIRED xcb-util)

add_executable(${PROJECT_NAME} 
  src/main.cpp
  src/keys.h
  src/actions.h
  src/config.h
  src/logging.h
  src/window_manager.h
  src/window_manager.cpp
  src/connection.h
  src/connection.cpp
  src/event.h
  src/event.cpp
)

set(SPD_LOG_DIR third_party/spdlog)
add_subdirectory(${SPD_LOG_DIR})

target_compile_definitions(${PROJECT_NAME} PRIVATE 
  SRC_DIR="${CMAKE_SOURCE_DIR}"
  SPDLOG_COMPILED_LIB
  # SPDLOG_USE_STD_FORMAT
)
target_compile_options(${PROJECT_NAME} PRIVATE 
  -fno-rtti -fno-exceptions # baseline

  -Wall
  -Wextra # reasonable and standard
  -Wshadow # warn the user if a variable declaration shadows one from a parent context
  -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps
  # catch hard to track down memory errors
  -Wold-style-cast # warn for c-style casts
  -Wcast-align # warn for potential performance problem casts
  -Wunused # warn on anything being unused
  -Woverloaded-virtual # warn if you overload (not override) a virtual function
  -Wpedantic # warn if non-standard C++ is used
  -Wconversion # warn on type conversions that may lose data
  -Wsign-conversion # warn on sign conversions
  -Wnull-dereference # warn if a null dereference is detected
  -Wdouble-promotion # warn if float is implicit promoted to double
  -Wformat=2 # warn on security issues around functions that format output (ie printf)
  -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation

  -fsanitize=address,leak,undefined
  -fno-omit-frame-pointer
  -O0 -g
  
  ${X11_CFLAGS}
  ${XCB_CFLAGS}
  ${XCB_KEYSYMS_CFLAGS}
)
target_link_options(${PROJECT_NAME} PRIVATE
  -fsanitize=address,leak,undefined
)

target_include_directories(${PROJECT_NAME} PRIVATE 
  ${X11_INCLUDE_DIRS}
  ${XCB_INCLUDE_DIRS}
  ${XCB_KEYSYMS_INCLUDE_DIRS}
  ${SPD_LOG_DIR}/include
)
target_link_libraries(${PROJECT_NAME} PRIVATE 
  ${X11_LIBRARIES}
  ${XCB_LIBRARIES}
  ${XCB_KEYSYMS_LIBRARIES}
  ${XCB_UTIL_LIBRARIES}
  spdlog::spdlog
)
target_precompile_headers(${PROJECT_NAME} PRIVATE
  <spdlog/spdlog.h>
)
