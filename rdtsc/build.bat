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
call cl /Zi /FC /c ..\rdtsc_utils.c || echo "Command Failed" && popd && exit /B
:: Link and create static lib
call lib rdtsc_utils.obj /OUT:librdtsc_utils.lib || echo "Command Failed" && popd && exit /B
echo ===============================================================

echo.
echo.
echo ===================
echo Compile Executables
echo ===================
:: Compile executables
call cl /Zi /FC ..\perf_test1.c librdtsc_utils.lib || echo "Command Failed" && popd && exit /B
call cl /Zi /FC ..\perf_test2.c librdtsc_utils.lib || echo "Command Failed" && popd && exit /B
call cl /Zi /FC ..\perf_test3.c librdtsc_utils.lib || echo "Command Failed" && popd && exit /B
call cl /Zi /FC ..\perf_test4.c librdtsc_utils.lib || echo "Command Failed" && popd && exit /B
call cl /Zi /FC ..\rdtsc_test.c librdtsc_utils.lib || echo "Command Failed" && popd && exit /B

echo.
echo.
echo ====================
echo Compile Completed OK
echo ====================
popd
