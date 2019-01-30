rem  Run this from within the top-level SV dir: deploy\win64\build-64.bat
rem  To build from clean, delete the folder build_win64_debug first

rem  NB you will probably also have to change the CONFIG in noconfig.pri
rem  from release to debug

echo on

set STARTPWD=%CD%

set QTDIR=C:\Qt\5.11.2\msvc2017_64
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

mkdir build_win64_debug
cd build_win64_debug

qmake -spec win32-msvc -r -tp vc ..\sonic-visualiser.pro
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild sonic-visualiser.sln /t:Build /p:Configuration=Debug
if %errorlevel% neq 0 exit /b %errorlevel%

copy .\checker\debug\vamp-plugin-load-checker.exe .\debug

copy %QTDIR%\bin\Qt5Cored.dll .\debug
copy %QTDIR%\bin\Qt5Guid.dll .\debug
copy %QTDIR%\bin\Qt5Widgetsd.dll .\debug
copy %QTDIR%\bin\Qt5Networkd.dll .\debug
copy %QTDIR%\bin\Qt5Xmld.dll .\debug
copy %QTDIR%\bin\Qt5Svgd.dll .\debug
copy %QTDIR%\bin\Qt5Testd.dll .\debug
copy %QTDIR%\plugins\platforms\qminimald.dll .\debug
copy %QTDIR%\plugins\platforms\qwindowsd.dll .\debug
copy %QTDIR%\plugins\styles\qwindowsvistastyled.dll .\debug
copy ..\sv-dependency-builds\win64-msvc\lib\libsndfile-1.dll .\debug

rem some of these expect to be run from the project root
cd ..
build_win64_debug\debug\test-svcore-base
if %errorlevel% neq 0 exit /b %errorlevel%
build_win64_debug\debug\test-svcore-system
if %errorlevel% neq 0 exit /b %errorlevel%
build_win64_debug\debug\test-svcore-data-fileio
if %errorlevel% neq 0 exit /b %errorlevel%
build_win64_debug\debug\test-svcore-data-model
if %errorlevel% neq 0 exit /b %errorlevel%

set PATH=%ORIGINALPATH%
