@echo off
REM ============================================================================
REM FastBLEOTA Firmware Uploader
REM 
REM Uploads the compiled firmware to the device via BLE OTA.
REM Edit DEVICE_ADDRESS below to match your device's MAC address.
REM ============================================================================

REM Change to the directory where this batch file is located
cd /d "%~dp0"

REM === CONFIGURATION ===
REM Set your device's BLE MAC address here (find with: python BLE_OTA.py --scan)
REM Default placeholder - override in secrets.bat (see secrets.bat.example)
set DEVICE_ADDRESS=AA:BB:CC:DD:EE:FF

REM Load secrets if secrets.bat exists (not committed to git)
if exist "secrets.bat" (
  call secrets.bat
)

REM Path to the compiled firmware binary
set FIRMWARE_PATH=.pio\build\um_nanos3\firmware.bin

REM Path to the BLE_OTA.py uploader
REM First try libdeps (GitHub dependency), then fall back to symlink location
set UPLOADER_PATH=.pio\libdeps\um_nanos3\FastBLEOTA\BLE_OTA.py
if not exist "%UPLOADER_PATH%" (
  set UPLOADER_PATH=..\..\library\FastBLEOTA\BLE_OTA.py
)

REM === RUN UPLOAD ===
echo.
echo === FastBLEOTA Upload ===
echo Device:   %DEVICE_ADDRESS%
echo Firmware: %FIRMWARE_PATH%
echo.

if not exist "%FIRMWARE_PATH%" (
  echo ERROR: Firmware not found at %FIRMWARE_PATH%
  echo Run 'pio run' first to build the firmware.
  pause
  exit /b 1
)

python "%UPLOADER_PATH%" -a %DEVICE_ADDRESS% -f "%FIRMWARE_PATH%" -v

echo.
pause
