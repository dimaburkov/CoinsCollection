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

rem Release checklist (task 46): the newest installer, its SHA-256 and the publishing steps.
set "FILE="
for /f "delims=" %%F in ('dir /b /o-d "%SETUP%Output\CoinsCollection-Setup-*.exe"') do if not defined FILE set "FILE=%%F"
echo.
echo Done: %SETUP%Output\%FILE%
echo.
echo SHA-256:
powershell -NoProfile -Command "(Get-FileHash -Algorithm SHA256 '%SETUP%Output\%FILE%').Hash"
echo.
echo Release checklist (Docs\Tech\CoinsCollection-tech.md, "Publikatsiya na GitHub"):
echo   1. SHA-256 above -^> Setup\ReleaseNotes\^<version^>.md
echo   2. Commit, merge to master, push
echo   3. GitHub: Releases -^> Draft a new release, tag v^<version^> on master, title, text from ReleaseNotes
echo   4. ASSETS: attach %FILE%  ^<-- do not forget
echo   5. Publish, then check: the installer is in Assets, downloads, SHA-256 matches
exit /b 0

:error
echo.
echo Build failed.
exit /b 1
