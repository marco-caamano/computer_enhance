;
; Use this for quick testing with the run test target
;

bits 16

mov [bp + di], byte 7
add word [bp + si + 1000], 29
mov [di + 901], word 347
add byte [bx], 34
