@echo off
cls
IF NOT EXIST build mkdir build
pushd build

echo.
echo.
echo ===================
echo Compile Executables
echo ===================
:: Compile executables
call cl /Zi /FC -I..\..\rdtsc\ -I..\..\rep_tester ..\page_faults1.c ..\..\rep_tester\build\libreptester.lib ..\..\rdtsc\build\librdtsc_utils.lib || echo "Command Failed" && popd && exit /B
call cl /Zi /FC -I..\..\rdtsc\ -I..\..\rep_tester ..\page_faults2.c ..\..\rep_tester\build\libreptester.lib ..\..\rdtsc\build\librdtsc_utils.lib || echo "Command Failed" && popd && exit /B
call cl /Zi /FC -I..\..\rdtsc\ ..\page_faults3.c ..\..\rdtsc\build\librdtsc_utils.lib || echo "Command Failed" && popd && exit /B
call cl /Zi /FC -I..\..\rdtsc\ ..\page_faults4.c ..\..\rdtsc\build\librdtsc_utils.lib || echo "Command Failed" && popd && exit /B

echo.
echo.
echo ====================
echo Compile Completed OK
echo ====================
popd
