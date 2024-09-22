cd /d "%~dp0"

python "BLE OTA.py" --address F4:12:FA:9F:0C:7D --file ".pio\build\um_nanos3\firmware.bin"
pause