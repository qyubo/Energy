@echo off
setlocal
set "NODE_EXE="
for %%I in (node.exe) do set "NODE_EXE=%%~$PATH:I"
if not defined NODE_EXE set "NODE_EXE=%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin\node.exe"
if not exist "%NODE_EXE%" (
  echo Node.js not found. Install Node.js 18+ or ask Codex to launch the local page.
  pause
  exit /b 1
)
"%NODE_EXE%" "%~dp0tools\serve-web.js"
pause
