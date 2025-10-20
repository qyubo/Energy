#THIS FILE IS AUTO GENERATED FROM THE TEMPLATE! DO NOT CHANGE!
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 4.0)

# specify cross-compilers and tools
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER  arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# project settings
project(RM26_F4 C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

#Uncomment for hardware floating point
add_compile_definitions(ARM_MATH_CM4;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)

#Uncomment for software floating point
#add_compile_options(-mfloat-abi=soft)

add_compile_options(-mcpu=cortex-m4 -mthumb -mthumb-interwork)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

# uncomment to mitigate c++17 absolute addresses warnings
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

# Enable assembler files preprocessing
add_compile_options($<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>)

add_compile_options(-O0 -g3)

include_directories(
        Core/Inc
        AZURE_RTOS/App Drivers/STM32F4xx_HAL_Driver/Inc
        Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
        Drivers/CMSIS/Device/ST/STM32F4xx/Include
        Drivers/CMSIS/Include
        Middlewares/ST/ARM/DSP/Inc
        Middlewares/ST/threadx/common/inc/
        Middlewares/ST/threadx/ports/cortex_m4/gnu/inc/
        USBX/App
        USBX/Target
        Middlewares/ST/usbx/common/core/inc/
        Middlewares/ST/usbx/ports/generic/inc/
        Middlewares/ST/usbx/common/usbx_stm32_device_controllers/
        Middlewares/ST/usbx/common/usbx_device_classes/inc/
        User/BSP/Inc
        User/Controller/Inc
        User/Module/AHRS/Inc
        User/Module/DJIMotor/Inc
        User/Module/DMMotor/Inc
        User/Module/IMU/Inc
        User/Module/LED
        User/Module/LKMotor/Inc
        User/Module/Logger
        User/Module/Referee
        User/Module/RemoteControl/Inc
        User/Module/SuperCap
        User/Service/Inc
        User/Task/Inc
        User/Utils/Inc
)

add_definitions(-DDEBUG -DTX_INCLUDE_USER_DEFINE_FILE -DUSE_HAL_DRIVER -DSTM32F407xx -DUX_INCLUDE_USER_DEFINE_FILE)

file(GLOB_RECURSE SOURCES
        "AZURE_RTOS/*.*"
        "USBX/*.*"
        "Core/*.*"
        "Middlewares/*.*"
        "Drivers/*.*"
        "User/*.*"
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32F407IGHX_FLASH.ld)

add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
add_link_options(-mcpu=cortex-m4 -mthumb -mthumb-interwork)
add_link_options(-T ${LINKER_SCRIPT})

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})

target_link_libraries(${PROJECT_NAME}.elf PUBLIC ${PROJECT_SOURCE_DIR}/DSP/libCMSISDSP.a)

# Add OneMessage
add_subdirectory(User/Module/OneMessage om)
target_include_directories(${PROJECT_NAME}.elf PUBLIC $<TARGET_PROPERTY:OneMessage,INCLUDE_DIRECTORIES>)
target_link_libraries(${PROJECT_NAME}.elf PUBLIC OneMessage)

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")
