@echo off
REM MindInk Test Script Setup - Windows One-Click Installation

echo ================================================
echo   MindInk Local Processing Test Setup
echo ================================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo ❌ Python is not installed
    echo    Please install Python 3.8 or higher from python.org
    pause
    exit /b 1
)

for /f "tokens=*" %%i in ('python --version') do set PYTHON_VERSION=%%i
echo ✅ Python found: %PYTHON_VERSION%
echo.

REM Check if pip is installed
pip --version >nul 2>&1
if errorlevel 1 (
    echo ❌ pip is not installed
    echo    Please upgrade Python installation
    pause
    exit /b 1
)

for /f "tokens=*" %%i in ('pip --version') do set PIP_VERSION=%%i
echo ✅ pip found: %PIP_VERSION%
echo.

REM Install dependencies
echo Installing Python dependencies...
pip install -r requirements.txt

if errorlevel 1 (
    echo ❌ Failed to install dependencies
    pause
    exit /b 1
)

echo ✅ Dependencies installed
echo.

REM Create .env if it doesn't exist
if not exist ".env" (
    echo Creating .env file from template...
    copy .env.example .env
    echo ✅ .env file created
    echo.
    echo ⚠️  IMPORTANT: Edit .env and add your API keys:
    echo    ELEVEN_LABS_API_KEY=sk_...
    echo    GEMINI_API_KEY=AIza_...
) else (
    echo ✅ .env file already exists
)

REM Create audio_cache directory
if not exist "audio_cache" (
    mkdir audio_cache
)
echo ✅ Cache directory created: ./audio_cache/
echo.

REM Final instructions
echo ================================================
echo   Setup Complete! ✅
echo ================================================
echo.
echo Next steps:
echo   1. Edit .env and add your API keys
echo   2. Run: python test_local_processing.py
echo.
echo For help, see:
echo   - QUICKSTART_REFERENCE.md
echo   - TEST_SCRIPT_README.md
echo.
pause
