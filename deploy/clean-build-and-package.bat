@rem  Run this from within the top-level dir: deploy\clean-build-and-package
@echo on

@echo(
@set YN=y
@set /p YN="Proceed to clean, rebuild, package, and sign [Yn] ?"

@if "%YN%" == "Y" set YN=y
@if "%YN%" neq "y" exit /b 3

@echo Proceeding
call .\deploy\win64\build-and-package.bat sign
if %errorlevel% neq 0 exit /b %errorlevel%

@set /p VERSION=<build_win64\version.h
@set VERSION=%VERSION:#define SV_VERSION "=%
set VERSION=%VERSION:"=%

mkdir packages
copy sonic-visualiser.msi packages\sonic-visualiser-%VERSION%-win32.msi
copy sonic-visualiser.msi packages\sonic-visualiser-%VERSION%-win64.msi

@echo(
@echo Done
