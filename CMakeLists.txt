cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME llcc68fsk_for_f103)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)
if(DEFINED APP_VECTOR_BASE)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    APP_VECTOR_BASE=${APP_VECTOR_BASE})
endif()

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_CURRENT_SOURCE_DIR}/app/packet.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/pool.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/radio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/serial.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/tick.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/llcc68/llcc68.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/llcc68/llcc68_driver_version.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/llcc68/llcc68_hal.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/llcc68
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# App A 构建完成后，从 ELF 提取串口升级所需的裸 BIN。
if(FW_LINKER_SCRIPT STREQUAL "linker/app_a.ld")
    get_filename_component(_arm_tool_dir "${CMAKE_C_COMPILER}" DIRECTORY)
    find_program(_arm_objcopy NAMES arm-none-eabi-objcopy
        HINTS "${_arm_tool_dir}" NO_DEFAULT_PATH REQUIRED)
    set(_app_bin "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin")
    add_custom_command(
        OUTPUT "${_app_bin}"
        COMMAND "${_arm_objcopy}" -O binary
            "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>" "${_app_bin}"
        DEPENDS ${CMAKE_PROJECT_NAME}
        COMMENT "生成 App A 串口升级 BIN"
        VERBATIM)
    add_custom_target(app_a_bin ALL DEPENDS "${_app_bin}")
endif()
