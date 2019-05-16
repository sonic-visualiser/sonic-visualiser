rem  Run this from within the top-level SV dir: deploy\win64\build-64.bat
rem  To build from clean, delete the folder build_win64 first

echo on

set STARTPWD=%CD%

set QTDIR=C:\Qt\5.12.2\msvc2017_64
if not exist %QTDIR% (
@   echo Could not find 64-bit Qt
@   exit /b 2
)

if not exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" (
@   echo "Could not find MSVC vars batch file"
@   exit /b 2
)

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64

set ORIGINALPATH=%PATH%
set PATH=%PATH%;C:\Program Files (x86)\SMLNJ\bin;%QTDIR%\bin

cd %STARTPWD%

call .\repoint install
if %errorlevel% neq 0 exit /b %errorlevel%

sv-dependency-builds\win64-msvc\bin\capnp -Isv-dependency-builds/win64-msvc/include compile --src-prefix=piper/capnp -osv-dependency-builds/win64-msvc/bin/capnpc-c++:piper-vamp-cpp/vamp-capnp piper/capnp/piper.capnp
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir build_win64
cd build_win64

qmake -spec win32-msvc -r -tp vc ..\sonic-visualiser.pro
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild sonic-visualiser.sln /t:Build /p:Configuration=Release
if %errorlevel% neq 0 exit /b %errorlevel%

copy .\checker\release\vamp-plugin-load-checker.exe .\release

copy %QTDIR%\bin\Qt5Core.dll .\release
copy %QTDIR%\bin\Qt5Gui.dll .\release
copy %QTDIR%\bin\Qt5Widgets.dll .\release
copy %QTDIR%\bin\Qt5Network.dll .\release
copy %QTDIR%\bin\Qt5Xml.dll .\release
copy %QTDIR%\bin\Qt5Svg.dll .\release
copy %QTDIR%\bin\Qt5Test.dll .\release
copy %QTDIR%\plugins\platforms\qminimal.dll .\release
copy %QTDIR%\plugins\platforms\qwindows.dll .\release
copy %QTDIR%\plugins\styles\qwindowsvistastyle.dll .\release
copy ..\sv-dependency-builds\win64-msvc\lib\libsndfile-1.dll .\release

rem some of these expect to be run from the project root
cd ..
build_win64\release\test-svcore-base
if %errorlevel% neq 0 exit /b %errorlevel%
build_win64\release\test-svcore-system
if %errorlevel% neq 0 exit /b %errorlevel%
build_win64\release\test-svcore-data-fileio
if %errorlevel% neq 0 exit /b %errorlevel%
build_win64\release\test-svcore-data-model
if %errorlevel% neq 0 exit /b %errorlevel%

set PATH=%ORIGINALPATH%
