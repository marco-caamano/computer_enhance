@echo off
IF NOT EXIST build mkdir build
pushd build

:: Compile Decoder exe
call cl /I ..\..\libdecoder_8086\ /Zi /FC ..\simulator.c ..\..\libdecoder_8086\build\libdecoder.lib  || echo "Command Failed" && popd && exit /B

popd
