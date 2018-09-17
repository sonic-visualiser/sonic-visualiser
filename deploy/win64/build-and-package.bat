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

@echo Rebuilding 32-bit

cd %STARTPWD%
del /q /s build_win32
call .\deploy\win64\build-32.bat
if %errorlevel% neq 0 exit /b %errorlevel%

@echo Rebuilding 64-bit

cd %STARTPWD%
del /q /s build_win64
call .\deploy\win64\build-64.bat
if %errorlevel% neq 0 exit /b %errorlevel%

set PATH=%PATH%;"C:\Program Files (x86)\WiX Toolset v3.11\bin"

@echo Packaging 32-bit

cd %STARTPWD%\build_win32
del sonic-visualiser.msi
candle -v ..\deploy\win32\sonic-visualiser.wxs
light -b . -ext WixUIExtension -ext WixUtilExtension -v sonic-visualiser.wixobj
if %errorlevel% neq 0 exit /b %errorlevel%
del sonic-visualiser.wixobj
del sonic-visualiser.wixpdb

@echo Packaging 64-bit

cd %STARTPWD%\build_win64
del sonic-visualiser.msi
candle -v ..\deploy\win64\sonic-visualiser.wxs
light -b . -ext WixUIExtension -ext WixUtilExtension -v sonic-visualiser.wixobj
if %errorlevel% neq 0 exit /b %errorlevel%
del sonic-visualiser.wixobj
del sonic-visualiser.wixpdb

@echo Done

