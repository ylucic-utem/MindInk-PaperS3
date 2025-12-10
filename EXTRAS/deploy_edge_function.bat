@echo off
REM Deploy process-audio edge function to Supabase

echo Deploying process-audio edge function...
echo.

REM Check if supabase CLI is installed
where supabase >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Supabase CLI not found!
    echo Install it with: npm install -g supabase
    exit /b 1
)

REM Deploy the function
cd supabase\functions\process-audio
supabase functions deploy process-audio --project-ref fptyrdjmrzabvimbzupv

if %errorlevel% equ 0 (
    echo.
    echo ========================================
    echo Deployment successful!
    echo ========================================
) else (
    echo.
    echo ========================================
    echo Deployment failed!
    echo ========================================
    exit /b 1
)
