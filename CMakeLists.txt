cmake_minimum_required(VERSION 3.10)
project(chip8-cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    src/interface.cpp
    src/chip8.cpp
)

# Add header files
set(HEADERS
    src/chip8.h
)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link SDL2
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

# Enable all warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
elseif (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()
