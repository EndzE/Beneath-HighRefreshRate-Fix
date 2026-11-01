Write-Host "Building Beneath High Refresh Rate Fix (dxgi.dll)..." -ForegroundColor Cyan
& gcc -O2 -shared -o dxgi.dll src/dxgi_proxy.c -static -static-libgcc
if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful: dxgi.dll" -ForegroundColor Green
} else {
    Write-Host "Build failed with exit code $LASTEXITCODE" -ForegroundColor Red
}
