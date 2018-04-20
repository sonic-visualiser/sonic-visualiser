@ECHO OFF
SET WIXPATH="C:\Program Files (x86)\WiX Toolset v3.11\bin"
IF NOT EXIST %WIXPATH% (
    SET WIXPATH="C:\Program Files\WiX Toolset v3.11\bin"
)
DEL sonic-visualiser.msi
%WIXPATH%\candle.exe -v sonic-visualiser.wxs
%WIXPATH%\light.exe -b ..\..\..\build-sonic-visualiser-Desktop_Qt_5_10_1_MinGW_32bit-Release -ext WixUIExtension -ext WixUtilExtension -v sonic-visualiser.wixobj
PAUSE
DEL sonic-visualiser.wixobj
DEL sonic-visualiser.wixpdb
