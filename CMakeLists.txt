cmake_minimum_required(VERSION 3.15)
project(jsonview)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation directory" FORCE)
endif()

message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

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

# imgui
file(GLOB IMGUI_SOURCES "lib/imgui/*.cpp")
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui>
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
add_executable(jsonview src/main.cpp)
target_link_libraries(jsonview MyLib)


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
    # PATTERN "*.cpp"  # imgui通常需要cpp文件一起安装
)


install(DIRECTORY lib/glad2/include/ DESTINATION include)

install(DIRECTORY lib/glfw/include/GLFW/ DESTINATION include/GLFW)
