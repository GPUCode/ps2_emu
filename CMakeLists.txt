cmake_minimum_required(VERSION 3.10)

# Set the project name
set(PROJECT_NAME gcnemu)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:10000000")

project(${PROJECT_NAME})

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Create executable target
add_executable(${PROJECT_NAME}	
	"src/main.cc"
	"src/common/emulator.cc"
	"src/cpu/ee/ee.cc"
	"src/cpu/ee/timers.cc"
	"src/cpu/iop/iop.cc"
	"src/cpu/iop/dma.cc" 
	"src/cpu/iop/timers.cc" 
	"src/cpu/ee/intc.cc"
	"src/cpu/iop/intr.cc"
	"src/gs/gif.cc"
	"src/gs/gs.cc"
	"src/cpu/ee/dmac.cc"
	"src/cpu/vu/vu0.cc"
	"src/cpu/vu/vif.cc"
	"src/media/ipu.cc"
	"src/cpu/ee/cop1.cc"
	"src/common/sif.cc"
	"src/spu/spu.cc"
)

# Link to all the required conan libs
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
