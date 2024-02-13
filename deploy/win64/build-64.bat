rem  Run this from within the top-level SV dir: deploy\win64\build-64.bat
rem  To build from clean, delete the folder build_win64 first

echo on

set STARTPWD=%CD%

rem The first path is for workstation builds, the others for CI
set QTDIR=C:\Qt\6.6.1\msvc2019_64
if not exist %QTDIR% (
    set QTDIR=%QT_ROOT_DIR%
)
if not exist %QTDIR% (
@   echo Could not find Qt in %QTDIR%
@   exit /b 2
)

rem Similarly, the first path is for workstation builds, the second for CI
set vcvarsall="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
if not exist %vcvarsall% (
    set vcvarsall="C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
)
if not exist %vcvarsall% (
@   echo Could not find MSVC vars batch file in %vcvarsall%
@   exit /b 2
)

call %vcvarsall% amd64

set ORIGINALPATH=%PATH%
set PATH=C:\Program Files (x86)\SMLNJ\bin;%QTDIR%\bin;%PATH%

cd %STARTPWD%

call .\repoint install
if %errorlevel% neq 0 exit /b %errorlevel%

set BUILDDIR=build_win64

if not exist %BUILDDIR%\build.ninja (
  meson setup %BUILDDIR% --buildtype release -Db_lto=true
  if %errorlevel% neq 0 exit /b %errorlevel%
)

ninja -C %BUILDDIR%
if %errorlevel% neq 0 exit /b %errorlevel%

copy %QTDIR%\bin\Qt6Core.dll .\%BUILDDIR%
copy %QTDIR%\bin\Qt6Gui.dll .\%BUILDDIR%
copy %QTDIR%\bin\Qt6Widgets.dll .\%BUILDDIR%
copy %QTDIR%\bin\Qt6Network.dll .\%BUILDDIR%
copy %QTDIR%\bin\Qt6Xml.dll .\%BUILDDIR%
copy %QTDIR%\bin\Qt6Svg.dll .\%BUILDDIR%
copy %QTDIR%\bin\Qt6Test.dll .\%BUILDDIR%

mkdir .\%BUILDDIR%\plugins
mkdir .\%BUILDDIR%\plugins\platforms
mkdir .\%BUILDDIR%\plugins\styles

copy %QTDIR%\plugins\platforms\qdirect2d.dll .\%BUILDDIR%\plugins\platforms
copy %QTDIR%\plugins\platforms\qminimal.dll .\%BUILDDIR%\plugins\platforms
copy %QTDIR%\plugins\platforms\qoffscreen.dll .\%BUILDDIR%\plugins\platforms
copy %QTDIR%\plugins\platforms\qwindows.dll .\%BUILDDIR%\plugins\platforms
copy %QTDIR%\plugins\styles\qwindowsvistastyle.dll .\%BUILDDIR%\plugins\styles

copy sv-dependency-builds\win64-msvc\lib\libsndfile-1.dll .\%BUILDDIR%

meson test -C %BUILDDIR%
if %errorlevel% neq 0 exit /b %errorlevel%

set PATH=%ORIGINALPATH%
