@echo off
setlocal
cd /d "%~dp0"

echo ==========================================
echo RingPass - First Test
echo ==========================================
echo.

echo [1/2] Running cross-bitness self-test...
RingPassSelfTest.exe
set SELFTEST=%ERRORLEVEL%

echo.
echo [2/2] Creating diagnostic report...
RingPassDoctor.exe
set DOCTOR=%ERRORLEVEL%

echo.
if "%SELFTEST%"=="0" (
  echo SELF TEST: PASS
) else (
  echo SELF TEST: FAIL ^(code %SELFTEST%^)
)

if "%DOCTOR%"=="0" (
  echo DIAGNOSTIC: CREATED
  echo File: RingPass-Diagnostic.txt
) else (
  echo DIAGNOSTIC: FAIL ^(code %DOCTOR%^)
)

echo.
echo You can send RingPass-Diagnostic.txt back for analysis.
echo.
pause
