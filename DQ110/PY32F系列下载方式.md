# PY32F系列下载方式

日期：2026-10-29

## 用途

用于 DQ110 的 PY32F002B 固件快速下载。以后不需要手动输入探头 UID，也不需要分开执行编译、传输和下载命令。

## 直接使用

在工程根目录执行：

    bash tools/download_dq110.sh

脚本名称：tools/download_dq110.sh

## 自动完成的内容

1. 编译当前 build-002b 固件。
2. 将 dq110_fw.elf 传输到 Ubuntu。
3. 在同一次 SSH 调用中自动读取当前在线的 DAPLink UID，不需要单独确认权限或手动输入 UID。
4. 使用 py32f002bx5 和 1.2.8.pack 下载。
5. 复位目标板、释放运行并读取运行状态。

## 一次权限范围

首次执行时允许该下载方式的外部访问即可。后续一次调用完成本地编译、SCP 传输、UID读取和 SSH 下载，不再重复手动申请每一个步骤的权限。

默认连接：

- Ubuntu：andrew@10.211.55.4
- pyOCD：/home/andrew/ARTERY/.tools/pyocd-venv/bin/python
- 兼容脚本：/tmp/dq110_pyocd_hid_compat.py
- 固件包：/home/andrew/ARTERY/DQ110_FW/1.2.8.pack
- 目标芯片：py32f002bx5
- 默认 SWD 频率：1M
- 默认连接模式：under-reset

## 注意事项

- 下载前 DAPLink 必须已经通过 Parallels 连接到 Ubuntu。
- 下载完成后目标板已进入运行状态，可以拔掉 SWD 线；但必须保留电池或 DC 电源。
- 如果新板通信不稳定，可降低 SWD 频率：

      DQ110_SWD_FREQ=100k bash tools/download_dq110.sh

- 如果需要手动指定连接模式：

      DQ110_CONNECT_MODE=under-reset bash tools/download_dq110.sh

- 如果提示没有识别到 DAPLink，先恢复 USB 转接，再重新执行同一条命令。
