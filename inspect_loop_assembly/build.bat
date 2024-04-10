@echo off
cls
IF NOT EXIST build mkdir build
pushd build

echo.
echo.
echo ====================
echo Compile Library Code
echo ====================
:: Compile library code
call cl /Zi /FC /c -I..\..\rdtsc\ ..\..\rep_tester\reptester.c || echo "Command Failed" && popd && exit /B
:: Link and create static lib
call lib reptester.obj /OUT:libreptester.lib || echo "Command Failed" && popd && exit /B
echo ===============================================================

echo.
echo.
echo ===================
echo Compile Executables
echo ===================
:: Compile executables
call cl /O1 /Zi /FC -I..\..\rdtsc\ -I..\..\rep_tester\ ..\fileread_reptest.c libreptester.lib ..\..\rdtsc\build\librdtsc_utils.lib || echo "Command Failed" && popd && exit /B

echo.
echo.
echo ====================
echo Compile Completed OK
echo ====================
popd
