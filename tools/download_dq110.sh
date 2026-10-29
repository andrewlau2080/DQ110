#!/usr/bin/env bash
set -euo pipefail

# 下载方式名称：PY32F系列下载方式

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${DQ110_BUILD_DIR:-$ROOT_DIR/build-002b}"
REMOTE_HOST="${DQ110_REMOTE_HOST:-andrew@10.211.55.4}"
REMOTE_PYTHON="${DQ110_REMOTE_PYTHON:-/home/andrew/ARTERY/.tools/pyocd-venv/bin/python}"
REMOTE_COMPAT="${DQ110_REMOTE_COMPAT:-/tmp/dq110_pyocd_hid_compat.py}"
REMOTE_PACK="${DQ110_REMOTE_PACK:-/home/andrew/ARTERY/DQ110_FW/1.2.8.pack}"
REMOTE_ELF="${DQ110_REMOTE_ELF:-/home/andrew/ARTERY/DQ110_FW/dq110_fw_direct_adc.elf}"
SWD_FREQ="${DQ110_SWD_FREQ:-1M}"
CONNECT_MODE="${DQ110_CONNECT_MODE:-under-reset}"

LOCAL_ELF="$BUILD_DIR/dq110_fw.elf"

echo "PY32F系列下载方式"
echo "[1/3] 编译当前固件"
cmake --build "$BUILD_DIR" -j2

echo "[2/3] 传输固件到 Ubuntu"
scp "$LOCAL_ELF" "$REMOTE_HOST:$REMOTE_ELF"

echo "[3/3] 自动识别 DAPLink、下载并运行"
ssh "$REMOTE_HOST" bash -s -- \
  "$REMOTE_PYTHON" "$REMOTE_COMPAT" "$REMOTE_PACK" "$REMOTE_ELF" "$SWD_FREQ" "$CONNECT_MODE" <<'REMOTE_SCRIPT'
set -euo pipefail

PYTHON="$1"
COMPAT="$2"
PACK="$3"
ELF="$4"
FREQ="$5"
MODE="$6"

LIST="$(PYOCD_USB_BACKEND=hidapiusb "$PYTHON" "$COMPAT" list)"
printf '%s\n' "$LIST"

PROBE_UID="$(printf '%s\n' "$LIST" | grep -Eo '[[:alnum:]]{20,}' | head -n 1 || true)"
if [ -z "$PROBE_UID" ]; then
    echo "错误：没有识别到可用的 DAPLink 仿真器。" >&2
    exit 2
fi

echo "使用 DAPLink UID: $PROBE_UID"
set +e
FLASH_OUTPUT="$(PYOCD_USB_BACKEND=hidapiusb "$PYTHON" "$COMPAT" commander \
    -u "$PROBE_UID" \
    -t py32f002bx5 \
    --pack "$PACK" \
    -f "$FREQ" \
    -M "$MODE" \
    -c "load $ELF" \
    -c "reset halt" \
    -c "g" \
    -c "status" 2>&1)"
FLASH_STATUS=$?
set -e
printf '%s\n' "$FLASH_OUTPUT"
if [ "$FLASH_STATUS" -ne 0 ] ||
   printf '%s\n' "$FLASH_OUTPUT" | grep -Eqi 'Error:|read thread exited unexpectedly|not halted as expected'; then
    echo "错误：pyOCD下载未完成。" >&2
    exit 3
fi
REMOTE_SCRIPT

echo "下载完成，目标板已进入运行状态。"
