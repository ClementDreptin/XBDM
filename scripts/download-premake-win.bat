@ECHO OFF

SET RootDir=%~dp0..
SET PremakeDir=%RootDir%\tools\premake
SET PremakeArchivePath=%PremakeDir%\tmp.zip
SET PremakeVersion=5.0.0-beta2

REM Download the premake archive from GitHub
POWERSHELL -Command "Invoke-WebRequest https://github.com/premake/premake-core/releases/download/v%PremakeVersion%/premake-%PremakeVersion%-windows.zip -OutFile '%PremakeArchivePath%'"

REM Extract the executable from the archive
POWERSHELL -Command "Expand-Archive '%PremakeArchivePath%' '%PremakeDir%'"

REM Delete the premake archive
DEL "%PremakeArchivePath%"
