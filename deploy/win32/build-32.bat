rem  Run this from within the top-level SV dir: deploy\win32\build-32.bat
rem  To build from clean, delete the folder build_win32

echo on

set STARTPWD=%CD%

set QTDIR=C:\Qt\5.11.3\mingw53_32
if not exist %QTDIR% (
@   echo Could not find 32-bit Qt
@   exit /b 2
)

set ORIGINALPATH=%PATH%
set PATH=C:\Program Files (x86)\SMLNJ\bin;%QTDIR%\bin;C:\Qt\Tools\QtCreator\bin;C:\Qt\Tools\mingw530_32\bin;%PATH%

cd %STARTPWD%

call .\repoint install
if %errorlevel% neq 0 exit /b %errorlevel%

if not exist build_win32 (
  meson build_win32 --buildtype release
  if %errorlevel% neq 0 exit /b %errorlevel%
)

ninja -C build_win32
if %errorlevel% neq 0 exit /b %errorlevel%

copy %QTDIR%\bin\Qt5Core.dll .\build_win32
copy %QTDIR%\bin\Qt5Gui.dll .\build_win32
copy %QTDIR%\bin\Qt5Widgets.dll .\build_win32
copy %QTDIR%\bin\Qt5Network.dll .\build_win32
copy %QTDIR%\bin\Qt5Xml.dll .\build_win32
copy %QTDIR%\bin\Qt5Svg.dll .\build_win32
copy %QTDIR%\bin\Qt5Test.dll .\build_win32
copy %QTDIR%\bin\libgcc_s_dw2-1.dll .\build_win32
copy %QTDIR%\bin\"libstdc++-6.dll" .\build_win32
copy %QTDIR%\bin\libwinpthread-1.dll .\build_win32
copy %QTDIR%\plugins\platforms\qminimal.dll .\build_win32
copy %QTDIR%\plugins\platforms\qwindows.dll .\build_win32
copy %QTDIR%\plugins\styles\qwindowsvistastyle.dll .\build_win32

meson test -C build_win32
if %errorlevel% neq 0 exit /b %errorlevel%

set PATH=%ORIGINALPATH%
