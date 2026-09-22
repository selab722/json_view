cmake_minimum_required(VERSION 3.15)
project(jsonview)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation directory" FORCE)
endif()

message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

# 源文件是无 BOM 的 UTF-8，且含中文字面量（ImGui 要求 UTF-8）。
# MSVC 默认按本地代码页（如 936）解析源文件，并把窄字符串转成该代码页，会导致：
#   1) warning C4819；
#   2) 部分汉字的尾字节是 0x5C（即反斜杠），在字符串字面量中被当成转义符，
#      报 error C2001 "字符串字面量中的换行符" 以及后续连锁语法错误；
#   3) 界面中文乱码（窄字符串变成 GBK，而 ImGui 期望 UTF-8）。
# 因此对含中文的目标显式加 /utf-8，把源字符集与执行字符集统一为 UTF-8。
if(MSVC)
    set(AVE_UTF8_COMPILE_OPTIONS /utf-8)
else()
    set(AVE_UTF8_COMPILE_OPTIONS "")
endif()

file(GLOB_RECURSE SOURCES "src/**/*.cpp")

# message("=== SOURCES ===")
# foreach(source ${SOURCES})
#     message("${source}")
# endforeach()

# MyLib
add_library(MyLib STATIC ${SOURCES})
target_include_directories(MyLib PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)
target_compile_options(MyLib PRIVATE ${AVE_UTF8_COMPILE_OPTIONS})

# imgui
set(IMGUI_SOURCES
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
    lib/imgui/misc/cpp/imgui_stdlib.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/backends>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/misc/cpp>
    $<INSTALL_INTERFACE:include>
)

# glfw
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build GLFW as static library" FORCE)
add_subdirectory(lib/glfw)

# glad2
add_library(glad2 lib/glad2/src/gl.c)
target_include_directories(glad2 PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/glad2/include>
    $<INSTALL_INTERFACE:include>
)


target_link_libraries(MyLib imgui)
target_link_libraries(imgui glfw)
target_link_libraries(imgui glad2)



# executable
add_executable(test1 src/test1.cpp)
target_link_libraries(test1 MyLib)

add_executable(test2 src/test2.cpp)
target_link_libraries(test2 MyLib)


# ========== Install configuration ==========

install(TARGETS MyLib imgui glad2 glfw
    EXPORT jsonview-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)


install(DIRECTORY src/ DESTINATION include 
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.inl"
)


install(DIRECTORY lib/imgui/ DESTINATION include
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "backends" EXCLUDE  # 排除子目录
)


install(DIRECTORY lib/imgui/backends/ DESTINATION include
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp"
)


install(DIRECTORY lib/glad2/include/ DESTINATION include)

install(DIRECTORY lib/glfw/include/GLFW/ DESTINATION include/GLFW)
