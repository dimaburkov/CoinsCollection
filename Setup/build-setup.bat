@echo off
rem Coins Collection - build the installer (task 23).
rem 1) Release without runtime packages -> Setup\Build (exe, Lang, Help - project post-build)
rem 2) Inno Setup 5 (ISCC) -> Setup\Output\CoinsCollection-Setup-<version>.exe
setlocal
set "SETUP=%~dp0"
set "RSVARS=C:\Program Files (x86)\Embarcadero\Studio\37.0\bin\rsvars.bat"
set "ISCC=C:\Program Files (x86)\Inno Setup 5\ISCC.exe"

call "%RSVARS%" || goto :error
msbuild "%SETUP%..\Source\_CoinsCollection.cbproj" /t:Rebuild /p:Config=Release /p:Platform=Win32 "/p:FinalOutputDir=%SETUP%Build\\" /v:minimal /nologo || goto :error
"%ISCC%" /Q "%SETUP%CoinsCollection.iss" || goto :error

echo.
echo Done: %SETUP%Output
exit /b 0

:error
echo.
echo Build failed.
exit /b 1
