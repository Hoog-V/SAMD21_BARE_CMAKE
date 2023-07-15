cmake_minimum_required(VERSION 3.17)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
# specify cross compilers and tools
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# project settings
project(ATMELSAMD21 C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(MCPU cortex-m0plus)


add_compile_options(-mcpu=${MCPU} -mthumb -mthumb-interwork)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)


if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-Og -g)
endif ()

include_directories(
        ${CMAKE_SOURCE_DIR}/samd21a/include
        ${CMAKE_SOURCE_DIR}/CMSIS/Core/Include
        ${CMAKE_SOURCE_DIR}/Universal_hall/hal_gpio/
        ${CMAKE_SOURCE_DIR}/Universal_hall/hal_gpio/platform/atmelsam/
        ${CMAKE_SOURCE_DIR}/Universal_hall/hal_i2c/
        ${CMAKE_SOURCE_DIR}/Universal_hall/hal_i2c/platform/atmelsam/
        ${CMAKE_SOURCE_DIR}/Universal_hall/platform_dep/atmelsam/
        .
        )

add_definitions( -D__SAMD21G18A__ -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin )

file(GLOB_RECURSE SOURCES
        "samd21a/gcc/gcc/*.*" "samd21a/gcc/*.*"
        "main.cpp"
        #[[TODO rest of cource locations]]
        )

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/samd21a/gcc/gcc/samd21g18a_flash.ld)

add_link_options(-lm -Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map --specs=nano.specs)
add_link_options(-mcpu=${MCPU} -mthumb -mthumb-interwork)
add_link_options(-T ${LINKER_SCRIPT})

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")