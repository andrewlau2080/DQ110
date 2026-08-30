# DQ110 双速直流马达驱动器 · 固件

PY32F002A (Cortex-M0+ / 24MHz) 固件：三档船型开关（0关/一档15RPM/二档30RPM）+ 单向单 MOS 低侧 PWM 驱动 + 过流/堵转保护 + IWDG 看门狗 + Stop 模式待机（≤5µA）。

两种模式（CMake 选项）：

| 模式 | 选项 | 转速控制 |
|---|---|---|
| 标准版 | 默认（DQ110_HALL=ON） | 霍尔闭环 PID 稳速 ±5% |
| 低成本版（可选） | -DDQ110_HALL=OFF | 无霍尔，开环线性调速 + 电压补偿恒速（满电 15/30RPM，电压下降自动加大 PWM；低压 2.0V 保护） |

规格依据：《DQ110_产品规格书.md》（7.4 TSSOP20 主设计 / 7.5 SOP8 低成本版）。

## 目录结构

```
DQ110_FW_GITHUB/
├── DQ110_FW/                 应用固件（本仓库内容）
│   ├── CMakeLists.txt
│   ├── py32f002ax5.ld        链接脚本（20KB Flash / 3KB RAM）
│   ├── startup_py32f002xx.s  启动汇编
│   ├── Inc/
│   │   ├── main.h
│   │   ├── py32f0xx_hal_conf.h   HAL 模块裁剪
│   │   ├── py32f0xx_it.h
│   │   ├── dq110.h               引脚映射（TSSOP20/SOP8 双封装）+ 参数 + 状态机
│   │   ├── dq110_gpio.h          SW1/SW2 档位检测 + EXTI 唤醒
│   │   ├── dq110_tim.h           TIM1 PWM + 霍尔输入捕获测速
│   │   ├── dq110_adc.h           过流检测 ADC
│   │   ├── dq110_speed.h         PID 稳速
│   │   ├── dq110_protect.h       过流/堵转保护 + IWDG
│   │   └── dq110_power.h         Stop 低功耗 + 状态机
│   └── Src/                   对应 .c
└── .vendor/PY32F0xx_Firmware   OpenPuya 官方固件库（git 忽略，不随仓库上传）
```

## 构建

依赖：arm-none-eabi-gcc、cmake、ninja。固件库需先放入 `.vendor/PY32F0xx_Firmware`（见下）。

```bash
cd DQ110_FW_GITHUB
# 默认 TSSOP20 主设计（xpack arm-none-eabi 工具链，见 DQ110_FW/cmake/arm-none-eabi-toolchain.cmake）
cmake -S DQ110_FW -B build -G Ninja \
  -DCMAKE_MAKE_PROGRAM=$(which ninja) \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/DQ110_FW/cmake/arm-none-eabi-toolchain.cmake
ninja -C build -j1

# 低成本版 SOP8（PY32F002AL15S）
cmake -S DQ110_FW -B build-sop8 -G Ninja \
  -DCMAKE_MAKE_PROGRAM=$(which ninja) \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/DQ110_FW/cmake/arm-none-eabi-toolchain.cmake \
  -DDQ110_PKG=SOP8
ninja -C build-sop8 -j1

# 低成本版（无霍尔，开环电压补偿恒速）——封装任意，-DDQ110_HALL=OFF
cmake -S DQ110_FW -B build-sop8-nohall -G Ninja \
  -DCMAKE_MAKE_PROGRAM=$(which ninja) \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/DQ110_FW/cmake/arm-none-eabi-toolchain.cmake \
  -DDQ110_PKG=SOP8 -DDQ110_HALL=OFF
ninja -C build-sop8-nohall -j1
```

产物：`build/dq110_fw.elf` / `.hex` / `.bin`

## 固件库获取（首次）

```bash
mkdir -p .vendor && cd .vendor
git clone --depth 1 --filter=blob:none --sparse https://github.com/OpenPuya/PY32F0xx_Firmware.git
cd PY32F0xx_Firmware
git sparse-checkout set Drivers Projects/PY32F002A-STK
```

## 功能模块与关键参数（标定项）

| 模块 | 实现 | 标定/确认项 |
|---|---|---|
| 档位检测 | SW 内部上拉低有效 + EXTI 唤醒 + 30ms 防抖 | — |
| PWM 驱动 | TIM1_CH1(PA4,AF13) 20kHz，0~100% | — |
| 霍尔测速（标准版） | TIM1_CH3(PB6,AF1) 捕获 + 更新中断 32 位扩展 | `DQ110_HALL_PPR`（每转脉冲数） |
| PID 稳速（标准版） | 位置式 PID，100ms 周期，积分/输出限幅 | `DQ110_PID_KP/KI/KD`、`DQ110_PWM_DUTY_MIN` |
| 电压补偿恒速（低成本版，-DDQ110_HALL=OFF） | 开环：PWM = 基础占空比 × 满电电压/当前电压；VREFINT 反推电池电压 | `DQ110_DUTY_BASE_G1/G2`（满电标定） |
| 低压保护（低成本版） | VREFINT 反推电压 < 2.0V（2 节碱性电池 1.0V/节截止）连续 2 次关断 | `DQ110_VBAT_LOW_MV` |
| 过流保护 | 10ms 采样 8 点平均 + 连续 4 次超阈值（响应 ≤100ms） | `DQ110_OC_AMP_GAIN`（运放增益）、`DQ110_ADC_VREF_MV` |
| 堵转保护（标准版） | 运行中 500ms 无霍尔脉冲（启动宽限 1s）；低成本版由过流保护承担 | `DQ110_STALL_MS` |
| IWDG | LSI/64 → 8s 超时，主循环喂狗 | — |
| 待机 | 0 档 Stop + LPR；进 Stop 前关 LSI 冻结 IWDG，EXTI 唤醒后恢复 | 实测待机电流 ≤5µA |

## 状态机

```
STANDBY --拨档--> RUN --回0档--> STANDBY(Stop模式)
                RUN --过流/堵转--> FAULT --回0档--> STANDBY
```

## 烧录

SWD：PA13(SWDIO) / PA14(SWCLK)。烧录方式见 ARTERY 工具链 pyocd 流程（`--erase sector`，勿全擦）。
