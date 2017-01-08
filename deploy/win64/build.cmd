@ECHO OFF
SET WIXPATH="C:\Program Files (x86)\WiX Toolset v3.9\bin"
IF NOT EXIST %WIXPATH% (
    SET WIXPATH="C:\Program Files\WiX Toolset v3.9\bin"
)
DEL sonic-visualiser.msi
%WIXPATH%\candle.exe -v sonic-visualiser.wxs
%WIXPATH%\light.exe -b ..\..\..\build-sonic-visualiser-Desktop_Qt_5_7_0_MSVC2015_64bit-Release -ext WixUIExtension -v sonic-visualiser.wixobj
PAUSE
DEL sonic-visualiser.wixobj
DEL sonic-visualiser.wixpdb
