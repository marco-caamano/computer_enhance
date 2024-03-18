IF NOT EXIST build mkdir build

call build.bat

set PATH=C:\Users\marco\AppData\Local\bin\NASM;%PATH%

pushd build

:: 
:: First create the assembly of the source files
:: 
nasm ../tests/listing_0037_single_register_mov.asm -o listing_0037_single_register_mov.bin || echo "Command Failed" && exit /B
nasm ../tests/listing_0038_many_register_mov.asm -o listing_0038_many_register_mov.bin || echo "Command Failed" && exit /B
nasm ../tests/listing_0039_more_movs.asm -o listing_0039_more_movs.bin || echo "Command Failed" && exit /B
nasm ../tests/listing_0040_challenge_movs.asm -o listing_0040_challenge_movs.bin || echo "Command Failed" && exit /B
nasm ../tests/listing_0041_add_sub_cmp_jnz.asm -o listing_0041_add_sub_cmp_jnz.bin || echo "Command Failed" && exit /B
nasm ../tests/listing_0045_challenge_register_movs.asm -o listing_0045_challenge_register_movs.bin || echo "Command Failed" && exit /B

:: 
:: Decoder the assembler files
:: 
decoder.exe listing_0037_single_register_mov.bin listing_0037_single_register_mov.decoded || echo "Command Failed" && exit /B
decoder.exe listing_0038_many_register_mov.bin listing_0038_many_register_mov.decoded || echo "Command Failed" && exit /B
decoder.exe listing_0039_more_movs.bin listing_0039_more_movs.decoded || echo "Command Failed" && exit /B
decoder.exe listing_0040_challenge_movs.bin listing_0040_challenge_movs.decoded || echo "Command Failed" && exit /B
decoder.exe listing_0041_add_sub_cmp_jnz.bin listing_0041_add_sub_cmp_jnz.decoded || echo "Command Failed" && exit /B
decoder.exe listing_0045_challenge_register_movs.bin listing_0045_challenge_register_movs.decoded || echo "Command Failed" && exit /B

:: 
:: Re-create assembly files from decoded assembly files
:: 

nasm listing_0037_single_register_mov.decoded -o listing_0037_single_register_mov.rebuilt || echo "Command Failed" && exit /B
nasm listing_0038_many_register_mov.decoded -o listing_0038_many_register_mov.rebuilt || echo "Command Failed" && exit /B
nasm listing_0039_more_movs.decoded -o listing_0039_more_movs.rebuilt || echo "Command Failed" && exit /B
nasm listing_0040_challenge_movs.decoded -o listing_0040_challenge_movs.rebuilt || echo "Command Failed" && exit /B
nasm listing_0041_add_sub_cmp_jnz.decoded -o listing_0041_add_sub_cmp_jnz.rebuilt || echo "Command Failed" && exit /B
nasm listing_0045_challenge_register_movs.decoded -o listing_0045_challenge_register_movs.rebuilt || echo "Command Failed" && exit /B

::
:: Compare Results
::

COMP /M listing_0037_single_register_mov.bin  listing_0037_single_register_mov.rebuilt || echo "Command Failed" && exit /B
COMP /M listing_0038_many_register_mov.bin  listing_0038_many_register_mov.rebuilt || echo "Command Failed" && exit /B
COMP /M listing_0039_more_movs.bin  listing_0039_more_movs.rebuilt || echo "Command Failed" && exit /B
COMP /M listing_0040_challenge_movs.bin  listing_0040_challenge_movs.rebuilt || echo "Command Failed" && exit /B
COMP /M listing_0041_add_sub_cmp_jnz.bin  listing_0041_add_sub_cmp_jnz.rebuilt || echo "Command Failed" && exit /B
COMP /M listing_0045_challenge_register_movs.bin  listing_0045_challenge_register_movs.rebuilt || echo "Command Failed" && exit /B

popd