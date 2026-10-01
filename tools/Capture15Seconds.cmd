@echo off
setlocal
cd /d "%~dp0"

echo ==========================================
echo RingPass - 15 second telemetry capture
echo ==========================================
echo.
echo After pressing a key:
echo - stand still briefly
echo - run
echo - turn
echo - jump
echo - homing attack if possible
echo - collect a ring
echo.
pause

RingPassCapture.exe 15

echo.
echo Finished.
echo Send RingPass-Capture.csv back for analysis.
echo.
pause
