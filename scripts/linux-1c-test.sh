#!/usr/bin/env bash
#
# Интеграционный прогон VanessaExt на реальной 1С:Предприятие под Linux.
#
# Ожидает, что 1С установлена (например, в /opt/1cv8), а в текущем каталоге
# лежат Config/ и VanessaExt.epf. Запускается из AppVeyor (job LinuxTests),
# но может быть запущен и вручную:
#
#   ONEC_BIN=/opt/1cv8/x86_64/8.3.27.2342/1cv8 \
#     bash scripts/linux-1c-test.sh VanessaExt.epf
#
set -euo pipefail

EPF="${1:-$PWD/${APPVEYOR_PROJECT_NAME:-VanessaExt}.epf}"

export LANG="${LANG:-ru_RU.UTF-8}"
export LC_ALL="${LC_ALL:-ru_RU.UTF-8}"

ONEC="${ONEC_BIN:-}"
if [ -z "$ONEC" ]; then
  ONEC=$(command -v 1cv8 || true)
fi
if [ -z "$ONEC" ]; then
  ONEC=$(find /opt/1cv8 -type f -name 1cv8 2>/dev/null | sort | tail -n1)
fi

if [ -z "$ONEC" ] || [ ! -x "$ONEC" ]; then
  echo "1C binary not found (looked in /opt/1cv8). Set ONEC_BIN." >&2
  exit 1
fi

ONEC_THIN="$(dirname "$ONEC")/1cv8c"
if [ ! -x "$ONEC_THIN" ]; then
  ONEC_THIN="$ONEC"
fi

echo "1C:       $ONEC"
echo "1C thin:  $ONEC_THIN"
echo "EPF:      $EPF"
echo "LANG:     $LANG"
ls -d /opt/1cv8/x86_64/* 2>/dev/null || true

if [ ! -f "$EPF" ]; then
  echo "EPF not found: $EPF" >&2
  exit 1
fi

# --- Xvfb, window manager, D-Bus -------------------------------------------

export DISPLAY="${DISPLAY:-:99}"
XVFB_PID=""
WM_PID=""
DBUS_PID=""

cleanup() {
  set +e
  [ -n "$WM_PID" ] && kill "$WM_PID" 2>/dev/null
  [ -n "$DBUS_PID" ] && kill "$DBUS_PID" 2>/dev/null
  [ -n "$XVFB_PID" ] && kill "$XVFB_PID" 2>/dev/null
}
trap cleanup EXIT

if ! xdpyinfo -display "$DISPLAY" >/dev/null 2>&1; then
  echo "Starting Xvfb on $DISPLAY"
  Xvfb "$DISPLAY" -screen 0 1920x1080x24 -nolisten tcp &
  XVFB_PID=$!
  for _ in $(seq 1 30); do
    if xdpyinfo -display "$DISPLAY" >/dev/null 2>&1; then
      break
    fi
    sleep 0.5
  done
fi

if ! xdpyinfo -display "$DISPLAY" >/dev/null 2>&1; then
  echo "Xvfb did not start on $DISPLAY" >&2
  exit 1
fi

if command -v openbox >/dev/null 2>&1; then
  echo "Starting openbox"
  openbox >/dev/null 2>&1 &
  WM_PID=$!
  sleep 1
fi

if command -v dbus-launch >/dev/null 2>&1; then
  eval "$(dbus-launch --sh-syntax)"
  DBUS_PID="${DBUS_SESSION_BUS_PID:-}"
fi

# --- 1C --------------------------------------------------------------------

DB="$PWD/database"
rm -rf "$DB"

run() {
  echo "+ $*"
  "$@"
}

echo "Creating infobase"
run "$ONEC" CREATEINFOBASE "File=$DB" /DisableStartupDialogs /DisableStartupMessages

echo "Loading configuration"
run "$ONEC" DESIGNER /F "$DB" /LoadConfigFromFiles "$PWD/Config" /UpdateDBCfg \
  /Out "$PWD/load.log" /DisableStartupDialogs /DisableStartupMessages

# Пользователь CI создаётся без загрузки внешней обработки, поэтому диалогов нет.
# Под ним 1С не показывает предупреждение безопасности при загрузке VanessaExt.epf.
echo "Preparing CI user without unsafe-action warnings"
timeout 300 "$ONEC_THIN" ENTERPRISE /F "$DB" \
  /DisableStartupDialogs /DisableStartupMessages \
  /Out "$PWD/prepare.log" /C prepare

echo "Installing VanessaExt AddIn"
timeout 900 "$ONEC_THIN" ENTERPRISE /F "$DB" /Execute "$EPF" /N CI \
  /DisableStartupDialogs /DisableStartupMessages /Out "$PWD/install.log" /C install

echo "Running autotests"
timeout 2400 "$ONEC_THIN" ENTERPRISE /F "$DB" /Execute "$EPF" /N CI \
  /DisableStartupDialogs /DisableStartupMessages /Out "$PWD/autotest.log" /C autotest

if [ ! -f success.txt ]; then
  echo "Autotest failed: success.txt not found" >&2
  if [ -f autotest.log ]; then
    tail -n 200 autotest.log || true
  fi
  exit 1
fi

echo "Autotest passed"
cat success.txt
