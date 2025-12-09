# Deploy process-audio edge function to Supabase

Write-Host "Deploying process-audio edge function..." -ForegroundColor Cyan
Write-Host ""

# Check if supabase CLI is installed
if (-not (Get-Command supabase -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: Supabase CLI not found!" -ForegroundColor Red
    Write-Host "Install it with: npm install -g supabase"
    exit 1
}

# Deploy the function
Push-Location supabase\functions
try {
    supabase functions deploy process-audio --project-ref fptyrdjmrzabvimbzupv
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Green
        Write-Host "Deployment successful!" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Red
        Write-Host "Deployment failed!" -ForegroundColor Red
        Write-Host "========================================" -ForegroundColor Red
        exit 1
    }
} finally {
    Pop-Location
}
