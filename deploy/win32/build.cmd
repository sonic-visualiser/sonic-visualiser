@ECHO OFF
SET WIXPATH="C:\Program Files (x86)\WiX Toolset v3.8\bin"
IF NOT EXIST %WIXPATH% (
    SET WIXPATH="C:\Program Files\WiX Toolset v3.8\bin"
)
DEL sonic-visualiser.msi
%WIXPATH%\candle.exe -v sonic-visualiser.wxs
%WIXPATH%\light.exe -b ..\.. -ext WixUIExtension -v sonic-visualiser.wixobj
PAUSE
DEL sonic-visualiser.wixobj
DEL sonic-visualiser.wixpdb
