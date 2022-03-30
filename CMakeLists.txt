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
    src/cpu/ee/opcode.cc
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
    src/gs/vulkan/context.cc
    src/gs/vulkan/window.cc
    src/gs/vulkan/buffer.cc
    src/gs/vulkan/texture.cc
    src/cpu/ee/jit/jit.cc
    src/cpu/ee/jit/ir.cc
)

set(HEADERS
    src/common/emulator.h
    src/cpu/ee/cop0.h
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
    src/gs/vulkan/common.h
    src/gs/vulkan/context.h
    src/gs/vulkan/window.h
    src/gs/vulkan/buffer.h
    src/gs/vulkan/texture.h
    src/cpu/ee/jit/jit.h
    src/cpu/ee/jit/ir.h
)

set(SHADERS
    src/shaders/vertex.glsl
    src/shaders/fragment.glsl
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${SHADERS})

# Asmjit
set(ASMJIT_STATIC TRUE)
add_subdirectory(${ASMJIT_DIR})
target_link_libraries(${PROJECT_NAME} asmjit::asmjit)

# Vulkan
function(add_shader TARGET SHADER STAGE)
    find_program(GLSLC glslc)

    set(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/${SHADER})
    set(current-output-path ${CMAKE_BINARY_DIR}/bin/shaders/${SHADER}.spv)

    # Add a custom command to compile GLSL to SPIR-V.
    get_filename_component(current-output-dir ${current-output-path} DIRECTORY)
    file(MAKE_DIRECTORY ${current-output-dir})

    add_custom_command(
           OUTPUT ${current-output-path}
           COMMAND ${GLSLC} -o ${current-output-path} -fshader-stage=${STAGE} --target-env=vulkan1.1 ${current-shader-path}
           DEPENDS ${current-shader-path}
           IMPLICIT_DEPENDS CXX ${current-shader-path}
           VERBATIM)

    # Make sure our build depends on this output.
    set_source_files_properties(${current-output-path} PROPERTIES GENERATED TRUE)
    target_sources(${TARGET} PRIVATE ${current-output-path})
endfunction(add_shader)

# Find the VulkanSDK
find_package(Vulkan REQUIRED)

if(NOT Vulkan_FOUND)
                # CMake may fail to locate the libraries but could be able to
                # provide some path in Vulkan SDK include directory variable
                # 'Vulkan_INCLUDE_DIRS', try to extract path from this.
                message(STATUS "Failed to locate Vulkan SDK, retrying again...")
endif()

add_shader(${PROJECT_NAME} vertex.glsl vertex)
add_shader(${PROJECT_NAME} fragment.glsl fragment)

# Link to all the required conan libs
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS} ${Vulkan_LIBRARIES} ${SHADERC_LIB})
