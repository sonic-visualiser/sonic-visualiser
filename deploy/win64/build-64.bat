rem  Run this from within the top-level SV dir: deploy\win64\build-64.bat
rem  To build from clean, delete the folder build_win64 first

echo on

set STARTPWD=%CD%

set QTDIR=C:\Qt\5.13.2\msvc2017_64
if not exist %QTDIR% (
@   echo Could not find 64-bit Qt in %QTDIR%
@   exit /b 2
)

set vcvarsall="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"

if not exist %vcvarsall% (
@   echo "Could not find MSVC vars batch file"
@   exit /b 2
)

call %vcvarsall% amd64

set ORIGINALPATH=%PATH%
set PATH=C:\Program Files (x86)\SMLNJ\bin;%QTDIR%\bin;%PATH%

cd %STARTPWD%

call .\repoint install
if %errorlevel% neq 0 exit /b %errorlevel%

if not exist build_win64 (
  meson build_win64 --buildtype release -Db_lto=true
  if %errorlevel% neq 0 exit /b %errorlevel%
)

ninja -C build_win64
if %errorlevel% neq 0 exit /b %errorlevel%

copy %QTDIR%\bin\Qt5Core.dll .\build_win64
copy %QTDIR%\bin\Qt5Gui.dll .\build_win64
copy %QTDIR%\bin\Qt5Widgets.dll .\build_win64
copy %QTDIR%\bin\Qt5Network.dll .\build_win64
copy %QTDIR%\bin\Qt5Xml.dll .\build_win64
copy %QTDIR%\bin\Qt5Svg.dll .\build_win64
copy %QTDIR%\bin\Qt5Test.dll .\build_win64
copy %QTDIR%\plugins\platforms\qminimal.dll .\build_win64
copy %QTDIR%\plugins\platforms\qwindows.dll .\build_win64
copy %QTDIR%\plugins\styles\qwindowsvistastyle.dll .\build_win64
copy sv-dependency-builds\win64-msvc\lib\libsndfile-1.dll .\build_win64

meson test -C build_win64
if %errorlevel% neq 0 exit /b %errorlevel%

set PATH=%ORIGINALPATH%
