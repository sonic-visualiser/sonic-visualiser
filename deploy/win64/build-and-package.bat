rem  Run this from within the top-level SV dir: deploy\win64\build-and-package.bat

set STARTPWD=%CD%

if not exist "C:\Program Files (x86)\SMLNJ\bin" (
@   echo Could not find SML/NJ, required for Repoint
@   exit /b 2
)

if not exist "C:\Program Files (x86)\WiX Toolset v3.11\bin" (
@   echo Could not find WiX Toolset
@   exit /b 2
)

set ORIGINALPATH=%PATH%
set PATH=C:\Program Files (x86)\Windows Kits\10\bin\x64;%PATH%
set NAME=Christopher Cannam

set ARG=%1
shift
if "%ARG%" == "sign" (
@   echo NOTE: sign option specified, will attempt to codesign exe and msi
@   echo NOTE: starting by codesigning an unrelated executable, so we know
@   echo NOTE: whether it'll work before doing the entire build
copy sv-dependency-builds\win64-msvc\bin\capnp.exe signtest.exe
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a signtest.exe
if errorlevel 1 exit /b %errorlevel%
signtool verify /pa signtest.exe
if errorlevel 1 exit /b %errorlevel%
del signtest.exe
@   echo NOTE: success
) else (
@   echo NOTE: sign option not specified, will not codesign anything
)

@echo(
@echo Rebuilding 32-bit

cd %STARTPWD%
del /q /s build_win32
call .\deploy\win32\build-32.bat
if %errorlevel% neq 0 exit /b %errorlevel%

if "%ARG%" == "sign" (
@echo Signing 32-bit executables and libraries
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a build_win32\*.exe build_win32\*.dll
)

@echo Rebuilding 64-bit

cd %STARTPWD%
del /q /s build_win64
call .\deploy\win64\build-64.bat
if %errorlevel% neq 0 exit /b %errorlevel%

if "%ARG%" == "sign" (
@echo Signing 64-bit executables and libraries
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a build_win64\*.exe build_win64\*.dll
)

powershell -NoProfile -ExecutionPolicy Bypass -Command "& 'deploy\win32\generate-wxs.ps1'"
if errorlevel 1 exit /b %errorlevel%

powershell -NoProfile -ExecutionPolicy Bypass -Command "& 'deploy\win64\generate-wxs.ps1'"
if errorlevel 1 exit /b %errorlevel%

set PATH=%PATH%;"C:\Program Files (x86)\WiX Toolset v3.11\bin"

mkdir packages

@echo Packaging 32-bit

cd %STARTPWD%
del sonic-visualiser.msi
candle -v deploy\win32\sonic-visualiser.wxs
light -b . -ext WixUIExtension -ext WixUtilExtension -v sonic-visualiser.wixobj
if %errorlevel% neq 0 exit /b %errorlevel%
del sonic-visualiser.wixobj
del sonic-visualiser.wixpdb

if "%ARG%" == "sign" (
@echo Signing 32-bit package
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a sonic-visualiser.msi
signtool verify /pa sonic-visualiser.msi
)

@echo Copying 32-bit package to packages dir
@set /p VERSION=<build_win32\version.h
@set VERSION=%VERSION:#define SV_VERSION "=%
set VERSION=%VERSION:"=%
copy sonic-visualiser.msi packages\sonic-visualiser-%VERSION%-win32.msi

@echo Packaging 64-bit

cd %STARTPWD%
del sonic-visualiser.msi
candle -v deploy\win64\sonic-visualiser.wxs
light -b . -ext WixUIExtension -ext WixUtilExtension -v sonic-visualiser.wixobj
if %errorlevel% neq 0 exit /b %errorlevel%
del sonic-visualiser.wixobj
del sonic-visualiser.wixpdb

if "%ARG%" == "sign" (
@echo Signing 64-bit package
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a sonic-visualiser.msi
signtool verify /pa sonic-visualiser.msi
)

@echo Copying 64-bit package to packages dir
@set /p VERSION=<build_win64\version.h
@set VERSION=%VERSION:#define SV_VERSION "=%
set VERSION=%VERSION:"=%
copy sonic-visualiser.msi packages\sonic-visualiser-%VERSION%-win64.msi

set PATH=%ORIGINALPATH%

cd %STARTPWD%
@echo Done

