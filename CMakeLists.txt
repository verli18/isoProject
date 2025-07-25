cmake_minimum_required(VERSION 3.15)
project(IsometricGame LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable C++11 for raylib
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED raylib)
include_directories(${RAYLIB_INCLUDE_DIRS})

# Add source files
set(SOURCES
    src/main.cpp
    src/3DVoxelGrid.cpp
    src/marchingCubes.cpp
    src/chunk.cpp
)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create executable
add_executable(IsometricGame ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYLIB_LIBRARIES})
# Add the raylib library path to rpath
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "${RAYLIB_LIBRARY_DIRS}"
    BUILD_WITH_INSTALL_RPATH TRUE
    SKIP_BUILD_RPATH FALSE
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static
)

# Sanitizers
if(ENABLE_ASAN OR ENABLE_UBSAN)
    include(CheckCCompilerFlag)
    if(ENABLE_ASAN)
        set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
        check_c_compiler_flag("-fsanitize=address" HAS_ASAN)
        if(HAS_ASAN)
            target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
            target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
        endif()
    endif()
    
    if(ENABLE_UBSAN)
        set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
        check_c_compiler_flag("-fsanitize=undefined" HAS_UBSAN)
        if(HAS_UBSAN)
            target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined)
            target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined)
        endif()
    endif()
endif()

# Set compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -g -Wall -Wextra -Wpedantic -Werror)
endif()
