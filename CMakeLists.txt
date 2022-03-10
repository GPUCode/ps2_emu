cmake_minimum_required(VERSION 3.10)

# Set the project name
set(PROJECT_NAME gcnemu)
set(QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP ON)
set(ASMJIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/asmjit")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fms-extensions")

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
	"src/cpu/vu/vu.cc"
	"src/cpu/vu/vif.cc"
	"src/media/ipu.cc"
	"src/cpu/ee/cop1.cc"
	"src/common/sif.cc"
	"src/spu/spu.cc"
	"src/media/cdvd.cc"
	"src/media/sio2.cc"
	"src/media/gamepad.cc"
	"src/gs/gsvram.cc"
	"src/gs/gsrenderer.cpp" 
	"src/cpu/ee/jit/jit.cc"
 "src/cpu/ee/jit/ir.h")

# Asmjit
set(ASMJIT_STATIC TRUE)
add_subdirectory(${ASMJIT_DIR})
target_link_libraries(${PROJECT_NAME} asmjit::asmjit)

# Link to all the required conan libs
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
