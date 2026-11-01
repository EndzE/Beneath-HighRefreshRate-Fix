@echo off
echo Building Beneath High Refresh Rate Fix (dxgi.dll)...
gcc -O2 -shared -o dxgi.dll src/dxgi_proxy.c -static -static-libgcc
if %ERRORLEVEL% EQU 0 (
    echo Build successful: dxgi.dll
) else (
    echo Build failed!
)
