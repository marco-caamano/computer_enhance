@echo off
IF NOT EXIST build mkdir build
pushd build

call cl -Zi -FC ..\decoder.c || echo "Command Failed" && popd && exit /B

popd
