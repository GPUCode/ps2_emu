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
set(SOURCES
    src/main.cc
    src/common/emulator.cc
    src/cpu/ee/ee.cc
    src/cpu/ee/timers.cc
    src/cpu/iop/iop.cc
    src/cpu/iop/dma.cc
    src/cpu/iop/timers.cc
    src/cpu/ee/intc.cc
    src/cpu/iop/intr.cc
    src/gs/gif.cc
    src/gs/gs.cc
    src/cpu/ee/dmac.cc
    src/cpu/vu/vu.cc
    src/cpu/vu/vif.cc
    src/media/ipu.cc
    src/cpu/ee/cop1.cc
    src/common/sif.cc
    src/spu/spu.cc
    src/media/cdvd.cc
    src/media/sio2.cc
    src/media/gamepad.cc
    src/gs/gsvram.cc
    src/gs/gsrenderer.cpp
    src/cpu/ee/jit/jit.cc
    src/cpu/ee/jit/ir.cc
)

set(HEADERS
    src/common/emulator.h
    src/cpu/ee/ee.h
    src/cpu/ee/timers.h
    src/cpu/ee/opcode.h
    src/cpu/iop/iop.h
    src/cpu/iop/dma.h
    src/cpu/iop/timers.h
    src/cpu/ee/intc.h
    src/cpu/iop/intr.h
    src/gs/gif.h
    src/gs/gs.h
    src/cpu/ee/dmac.h
    src/cpu/vu/vu.h
    src/cpu/vu/vif.h
    src/media/ipu.h
    src/cpu/ee/cop1.h
    src/common/sif.h
    src/spu/spu.h
    src/media/cdvd.h
    src/media/sio2.h
    src/media/gamepad.h
    src/gs/gsvram.h
    src/gs/gsrenderer.h
    src/cpu/ee/jit/jit.h
    src/cpu/ee/jit/ir.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Asmjit
set(ASMJIT_STATIC TRUE)
add_subdirectory(${ASMJIT_DIR})
target_link_libraries(${PROJECT_NAME} asmjit::asmjit)

# Link to all the required conan libs
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
