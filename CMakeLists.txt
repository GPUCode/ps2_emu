cmake_minimum_required(VERSION 3.10)

# Set the project name
set(PROJECT_NAME gcnemu)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

project(${PROJECT_NAME})

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Create executable target
add_executable(${PROJECT_NAME} src/main.cpp src/common/manager.cpp src/cpu/ee.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link to all the required conan libs
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
