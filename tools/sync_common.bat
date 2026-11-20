@echo off
setlocal
set "ROOT=%~dp0.."
for %%D in (ESP32_1 ESP32_2) do (
  if not exist "%ROOT%\firmware\%%D\src" mkdir "%ROOT%\firmware\%%D\src"
  copy /Y "%ROOT%\firmware\common\*.h" "%ROOT%\firmware\%%D\src\" >nul
  copy /Y "%ROOT%\firmware\common\*.cpp" "%ROOT%\firmware\%%D\src\" >nul
)
echo Shared modules copied to both Arduino sketches.
endlocal
