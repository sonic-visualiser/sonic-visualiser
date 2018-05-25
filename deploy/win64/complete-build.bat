rem  Run this from within the top-level SV dir: deploy\win64\complete-build.bat
rem  To build from clean, delete the folders build_win32 and build_win64 first

set STARTPWD=%CD%

if not exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" (
@   echo "Could not find MSVC vars batch file"
@   exit /b
)

set QTDIR32=C:\Qt\5.10.1\mingw53_32
set QTDIR64=C:\Qt\5.10.1\msvc2017_64
if not exist %QTDIR32% (
@   echo Could not find 32-bit Qt
@   exit /b
)
if not exist %QTDIR64% (
@   echo Could not find 64-bit Qt
@   exit /b
)

if not exist "C:\Program Files (x86)\SMLNJ\bin" (
@   echo Could not find SML/NJ, required for Repoint
@   exit /b
)

if not exist "C:\Program Files (x86)\WiX Toolset v3.11\bin" (
@   echo Could not find WiX Toolset
@   exit /b
)

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64

set NONQTPATH=%PATH%;"C:\Program Files (x86)\SMLNJ\bin";"C:\Program Files (x86)\WiX Toolset v3.11\bin"

cd %STARTPWD%

call .\repoint install

set QTDIR=%QTDIR32%
set PATH=%NONQTPATH%;%QTDIR%\bin;C:\Qt\Tools\QtCreator\bin;C:\Qt\Tools\mingw530_32\bin

sv-dependency-builds\win32-mingw\bin\capnp -Isv-dependency-builds/win32-mingw/include compile --src-prefix=piper/capnp -osv-dependency-builds/win32-mingw/bin/capnpc-c++:piper-cpp/vamp-capnp piper/capnp/piper.capnp

mkdir build_win32
cd build_win32

qmake -r ..\sonic-visualiser.pro

jom

copy .\checker\release\vamp-plugin-load-checker.exe .\release

copy %QTDIR%\bin\Qt5Core.dll .\release
copy %QTDIR%\bin\Qt5Gui.dll .\release
copy %QTDIR%\bin\Qt5Widgets.dll .\release
copy %QTDIR%\bin\Qt5Network.dll .\release
copy %QTDIR%\bin\Qt5Xml.dll .\release
copy %QTDIR%\bin\Qt5Svg.dll .\release
copy %QTDIR%\bin\libgcc_s_dw2-1.dll .\release
copy %QTDIR%\bin\"libstdc++-6.dll" .\release
copy %QTDIR%\bin\libwinpthread-1.dll .\release
copy %QTDIR%\plugins\platforms\qminimal.dll .\release
copy %QTDIR%\plugins\platforms\qwindows.dll .\release

del sonic-visualiser.msi
candle -v ..\deploy\win32\sonic-visualiser.wxs
light -b . -ext WixUIExtension -ext WixUtilExtension -v sonic-visualiser.wixobj
del sonic-visualiser.wixobj
del sonic-visualiser.wixpdb

cd %STARTPWD%

set QTDIR=%QTDIR64%
set PATH=%NONQTPATH%;%QTDIR%\bin

sv-dependency-builds\win64-msvc\bin\capnp -Isv-dependency-builds/win64-msvc/include compile --src-prefix=piper/capnp -osv-dependency-builds/win64-msvc/bin/capnpc-c++:piper-cpp/vamp-capnp piper/capnp/piper.capnp

mkdir build_win64
cd build_win64

qmake -r -tp vc ..\sonic-visualiser.pro

msbuild sonic-visualiser.sln /t:Build /p:Configuration=Release

copy .\checker\release\vamp-plugin-load-checker.exe .\release

copy %QTDIR%\bin\Qt5Core.dll .\release
copy %QTDIR%\bin\Qt5Gui.dll .\release
copy %QTDIR%\bin\Qt5Widgets.dll .\release
copy %QTDIR%\bin\Qt5Network.dll .\release
copy %QTDIR%\bin\Qt5Xml.dll .\release
copy %QTDIR%\bin\Qt5Svg.dll .\release
copy %QTDIR%\plugins\platforms\qminimal.dll .\release
copy %QTDIR%\plugins\platforms\qwindows.dll .\release
copy ..\sv-dependency-builds\win64-msvc\lib\libsndfile-1.dll .\release

del sonic-visualiser.msi
candle -v ..\deploy\win64\sonic-visualiser.wxs
light -b . -ext WixUIExtension -ext WixUtilExtension -v sonic-visualiser.wixobj
del sonic-visualiser.wixobj
del sonic-visualiser.wixpdb
