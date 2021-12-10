cmake_minimum_required(VERSION 3.10)

# Set the project name
set(PROJECT_NAME gcnemu)
set(CMAKE_CXX_STANDARD 20)

project(${PROJECT_NAME})

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Create executable target
add_executable(${PROJECT_NAME}	
	"src/main.cc" 
	"src/common/manager.cc" 
	"src/cpu/ee/ee.cc"
	"src/cpu/iop/iop.cc"
	"src/cpu/iop/dma.cc")

# Link to all the required conan libs
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
