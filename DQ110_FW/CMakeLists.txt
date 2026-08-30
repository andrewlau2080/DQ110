cmake_minimum_required(VERSION 3.20)

project(dq110_fw C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ============================================================
# 路径
# ============================================================
set(FW_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(PRJ_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(VENDOR_DIR "${PRJ_ROOT}/.vendor/PY32F0xx_Firmware" CACHE PATH "OpenPuya PY32F0xx_Firmware 路径")
set(HAL_DRV_DIR "${VENDOR_DIR}/Drivers/PY32F0xx_HAL_Driver")
set(CMSIS_DEV_DIR "${VENDOR_DIR}/Drivers/CMSIS/Device/PY32F0xx")
set(CMSIS_CORE_DIR "${VENDOR_DIR}/Drivers/CMSIS/Include")

# ============================================================
# 编译目标选择：
#   DQ110_PKG  = TSSOP20（默认）| SOP8
#   DQ110_HALL = ON（默认，霍尔闭环）| OFF（低成本版，无霍尔开环电压补偿恒速）
# ============================================================
set(DQ110_PKG "TSSOP20" CACHE STRING "DQ110 封装：TSSOP20（主设计）或 SOP8（低成本版）")
if(DQ110_PKG STREQUAL "SOP8")
  add_definitions(-DDQ110_PKG_SOP8)
  message(STATUS "DQ110 target: SOP8 (PY32F002AL15S)")
else()
  set(DQ110_PKG "TSSOP20")
  message(STATUS "DQ110 target: TSSOP20 (PY32F002AF15P)")
endif()

set(DQ110_HALL "ON" CACHE STRING "霍尔闭环：ON（默认）或 OFF（低成本版，无霍尔+电压补偿恒速）")
if(DQ110_HALL STREQUAL "OFF")
  add_definitions(-DDQ110_NO_HALL)
  message(STATUS "DQ110 mode: 低成本版（无霍尔，开环电压补偿恒速）")
else()
  set(DQ110_HALL "ON")
  message(STATUS "DQ110 mode: 标准版（霍尔闭环 PID 稳速）")
endif()

# ============================================================
# 源文件
# ============================================================
set(APP_SOURCES
  "${FW_DIR}/Src/main.c"
  "${FW_DIR}/Src/py32f0xx_it.c"
  "${FW_DIR}/Src/py32f0xx_hal_msp.c"
  "${FW_DIR}/Src/dq110_gpio.c"
  "${FW_DIR}/Src/dq110_tim.c"
  "${FW_DIR}/Src/dq110_adc.c"
  "${FW_DIR}/Src/dq110_speed.c"
  "${FW_DIR}/Src/dq110_protect.c"
  "${FW_DIR}/Src/dq110_power.c"
)

set(HAL_SOURCES
  "${HAL_DRV_DIR}/Src/py32f0xx_hal.c"
  "${HAL_DRV_DIR}/Src/py32f0xx_hal_rcc.c"
  "${HAL_DRV_DIR}/Src/py32f0xx_hal_rcc_ex.c"
  "${HAL_DRV_DIR}/Src/py32f0xx_hal_gpio.c"
  "${HAL_DRV_DIR}/Src/py32f0xx_hal_tim.c"
  "${HAL_DRV_DIR}/Src/py32f0xx_hal_tim_ex.c"
  "${HAL_DRV_DIR}/Src/py32f0xx_hal_adc.c"
  "${HAL_DRV_DIR}/Src/py32f0xx_hal_iwdg.c"
  "${HAL_DRV_DIR}/Src/py32f0xx_hal_pwr.c"
  "${HAL_DRV_DIR}/Src/py32f0xx_hal_cortex.c"
  "${HAL_DRV_DIR}/Src/py32f0xx_hal_flash.c"
)

set(CMSIS_SOURCES
  "${CMSIS_DEV_DIR}/Source/system_py32f0xx.c"
  "${FW_DIR}/startup_py32f002xx.s"
)

# ============================================================
# 编译选项
# ============================================================
add_executable(dq110_fw.elf ${APP_SOURCES} ${HAL_SOURCES} ${CMSIS_SOURCES})

target_compile_definitions(dq110_fw.elf PRIVATE
  USE_HAL_DRIVER
  PY32F002Ax5
)

target_compile_options(dq110_fw.elf PRIVATE
  -mcpu=cortex-m0plus
  -mthumb
  -Wall
  -fdata-sections
  -ffunction-sections
)

target_include_directories(dq110_fw.elf PRIVATE
  "${FW_DIR}/Inc"
  "${HAL_DRV_DIR}/Inc"
  "${CMSIS_DEV_DIR}/Include"
  "${CMSIS_CORE_DIR}"
)

# 链接脚本 + 链接选项
target_link_options(dq110_fw.elf PRIVATE
  -mcpu=cortex-m0plus
  -mthumb
  -specs=nano.specs
  -T${FW_DIR}/py32f002ax5.ld
  -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/dq110_fw.map,--cref
  -Wl,--gc-sections
)

# ============================================================
# 产物：hex / bin
# ============================================================
find_program(ARM_OBJCOPY arm-none-eabi-objcopy
  HINTS "$ENV{HOME}/qtprj/ARTERY/.tools/xpack-arm-none-eabi-gcc/bin"
  REQUIRED)

add_custom_command(TARGET dq110_fw.elf POST_BUILD
  COMMAND ${ARM_OBJCOPY} -O ihex  dq110_fw.elf dq110_fw.hex
  COMMAND ${ARM_OBJCOPY} -O binary -S dq110_fw.elf dq110_fw.bin
  COMMENT "Generating dq110_fw.hex / dq110_fw.bin"
)
