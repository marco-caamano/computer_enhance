#!/bin/bash

echo
echo "Running Tests"
echo

if [ ! -d tests ] ; then
    echo "ERROR: tests directory not found, are you executing from the righ path?"
    exit 1
fi

if [ ! -x decoder ] ; then
    echo "Did not find decoder, testing if we can build it"
    if [ -e Makefile ]; then
        echo "Building Decoder"
        make
    fi
    if [ ! -x decoder ] ; then
        echo "ERROR: decoder still not found, build decoder or execute from the decoder path"
        exit 1
    fi
fi

cd tests || { echo "Failed to cd into tests"; exit 1; }
for test_file in `ls listing*.asm`; do 
    echo "##--------------------------------------------------------"
    echo "## Processing Listing $test_file"
    echo "##     Generating $test_file Assembly"
    nasm $test_file -o $test_file.bin
    if [ $? -ne 0 ] ; then
        echo "## ERROR: Failed to Generate assembly[$test_file]"
        exit 1
    fi
    echo "##     Decoding $test_file.bin File"
    ../decoder -i $test_file.bin -o out_$test_file.asm 
    if [ $? -ne 0 ] ; then
        echo "## ERROR: Failed to decode assembly[$test_file.bin]"
        exit 1
    fi
    echo "##     Generating Assembly of Decoded File"
    nasm out_$test_file.asm -o out_$test_file.bin
    if [ $? -ne 0 ] ; then
        echo "## ERROR: Failed to Generate assembly of decoded file[out_$test_file.asm]"
        exit 1
    fi
    echo "##     Compare Binaries"
    cmp -s $test_file.bin out_$test_file.bin
    if [ $? -ne 0 ] ; then
        echo "## ERROR: Binaries Do Not Match for $test_file"
        echo "## ----[$test_file] Test Failed"
        exit 1
    fi
    echo "## OK - Successful decoder of $test_file"
    echo "##--------------------------------------------------------"
    echo
done

echo 
echo "##"
echo "## All Tests Passed OK"
echo "##"
echo
