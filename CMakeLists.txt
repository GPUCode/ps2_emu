cmake_minimum_required(VERSION 3.10)

# Set the project name
set(PROJECT_NAME gcnemu)
set(CMAKE_CXX_STANDARD 20)
set(CXX_MODULES_FLAGS -std=c++20 -fmodules-ts)

project(${PROJECT_NAME})

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Build modules for all our needed STL headers, so we can use the nice module syntax everywhere uwu
set(STL_HEADERS iostream)
add_custom_target(
    stl_modules
    COMMAND g++-11 -std=c++20 -fmodules-ts -x c++-system-header ${STL_HEADERS}
    VERBATIM
)

# Create executable target
add_executable(${PROJECT_NAME} src/main.cpp)
add_dependencies(${PROJECT_NAME} stl_modules)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(${PROJECT_NAME} PRIVATE ${CXX_MODULES_FLAGS})

# Use the Widgets module from Qt 5.
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
