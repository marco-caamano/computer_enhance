@echo off
IF NOT EXIST build mkdir build
pushd build

:: Compile library code
call cl /Zi /FC /c ..\libdecoder.c || echo "Command Failed" && popd && exit /B
:: Link and create static lib
call lib libdecoder.obj || echo "Command Failed" && popd && exit /B

:: Compile Decoder exe
call cl /Zi /FC ..\decoder.c libdecoder || echo "Command Failed" && popd && exit /B

popd
