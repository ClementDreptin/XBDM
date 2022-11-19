@ECHO OFF

SET RootDir=%~dp0..
SET PremakeExecutablePath=%RootDir%\tools\premake\premake5.exe

IF NOT EXIST "%PremakeExecutablePath%" (
    ECHO Downloading premake...
    CALL "%~dp0download-premake-win.bat"
)

IF "%1" == "--test" (
    CALL "%PremakeExecutablePath%" --file="%RootDir%\premake5.lua" vs2022 --test
) ELSE (
    CALL "%PremakeExecutablePath%" --file="%RootDir%\premake5.lua" vs2022
)

