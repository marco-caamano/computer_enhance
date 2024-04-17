@echo off
cls
IF NOT EXIST build mkdir build
pushd build
set PATH=C:\Users\marco\AppData\Local\bin\NASM;%PATH%

echo.
echo.
echo ====================
echo Compile Library Code
echo ====================
:: Compile library code
call cl /Zi /FC /c -I..\..\rdtsc\ ..\..\rep_tester\reptester.c || echo "Command Failed" && popd && exit /B
:: Link and create static lib
call lib reptester.obj /OUT:libreptester.lib || echo "Command Failed" && popd && exit /B
:: Compile library code
call cl /Zi /FC /c -I..\..\rdtsc\ ..\..\rdtsc\rdtsc_utils.c || echo "Command Failed" && popd && exit /B
:: Link and create static lib
call lib rdtsc_utils.obj /OUT:librdtsc_utils.lib || echo "Command Failed" && popd && exit /B
echo ===============================================================

echo.
echo.
echo =================
echo Assemble ASM code
echo =================
:: Compile library code
call nasm -f win64 ..\asm_code.asm -o asm_code.obj || echo "Command Failed" && popd && exit /B
:: Link and create static lib
call lib asm_code.obj /OUT:asm_code.lib || echo "Command Failed" && popd && exit /B
echo ===============================================================

echo.
echo.
echo ===================
echo Compile Executables
echo ===================
:: Compile executables
call cl -O1 /Zi /FC -I..\..\rdtsc\ -I..\..\rep_tester\ ..\asm_link_test.c libreptester.lib librdtsc_utils.lib asm_code.lib || echo "Command Failed" && popd && exit /B

echo.
echo.
echo ====================
echo Compile Completed OK
echo ====================
popd
