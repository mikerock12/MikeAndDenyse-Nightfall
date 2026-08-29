@echo off
cd /d "%~dp0game"
echo Abrindo Mike ^& Denyse: Nightfall em http://localhost:8088
start "" cmd /c "timeout /t 1 >nul && start http://localhost:8088"
python -m http.server 8088
