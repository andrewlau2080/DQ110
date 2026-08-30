# ARM Cortex-M0+ 交叉编译 toolchain（xPack GNU Arm Embedded GCC）
# 本机标准工具链：~/qtprj/ARTERY/.tools/xpack-arm-none-eabi-gcc
# （Homebrew 版 arm-none-eabi-gcc 16.2 存在 stdint.h sysroot 问题，勿用）
set(XPACK_GCC_DIR "$ENV{HOME}/qtprj/ARTERY/.tools/xpack-arm-none-eabi-gcc")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m0plus)

set(CMAKE_C_COMPILER   "${XPACK_GCC_DIR}/bin/arm-none-eabi-gcc")
set(CMAKE_ASM_COMPILER "${XPACK_GCC_DIR}/bin/arm-none-eabi-gcc")

# 交叉编译不跑链接测试（避免 -arch 等主机参数混入）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
