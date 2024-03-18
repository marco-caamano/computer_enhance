@echo off
IF NOT EXIST build mkdir build
pushd build

call cl -Zi -FC ..\decoder.c 

popd
