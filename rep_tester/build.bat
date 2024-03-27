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
call cl /Zi /FC /c -I..\..\rdtsc\ ..\reptester.c || echo "Command Failed" && popd && exit /B
:: Link and create static lib
call lib reptester.obj /OUT:libreptester.lib || echo "Command Failed" && popd && exit /B
echo ===============================================================

echo.
echo.
echo ===================
echo Compile Executables
echo ===================
:: Compile executables
call cl /Zi /FC -I..\..\rdtsc\ ..\rep_test1.c libreptester.lib ..\..\rdtsc\build\librdtsc_utils.lib || echo "Command Failed" && popd && exit /B
:: call cl /Zi /FC -I..\..\rdtsc\ ..\rep_test2.c libreptester.lib || echo "Command Failed" && popd && exit /B
:: call cl /Zi /FC -I..\..\rdtsc\ ..\rep_test3.c libreptester.lib || echo "Command Failed" && popd && exit /B
:: call cl /Zi /FC -I..\..\rdtsc\ ..\rep_test4.c libreptester.lib || echo "Command Failed" && popd && exit /B

echo.
echo.
echo ====================
echo Compile Completed OK
echo ====================
popd
