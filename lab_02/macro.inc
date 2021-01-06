print_str macro str ; вывод строки на экран
    mov ah, 09h
    lea dx, str
    int 21h
endm

wait_key macro ; ожидание ввода символа с клавиатуры
    push eax
    mov ah, 10h
    int 16h
    pop eax
endm

clear_screen macro ; очистить экран
    mov	ax, 3
    int	10h
endm

memory_string macro 
    mov dI, 0
    mov ah, 00000010b
    mov al, 'M'
    stosw
    mov al, 'e'
    stosw
    mov al, 'm'
    stosw
    mov al, 'o'
    stosw
    mov al, 'r'
    stosw
    mov al, 'y'
    stosw
    mov al, ' '
    stosw
    mov al, 'a'
    stosw
    mov al, 'v'
    stosw
    mov al, 'a'
    stosw
    mov al, 'i'
    stosw
    mov al, 'l'
    stosw
    mov al, 'a'
    stosw
    mov al, 'b'
    stosw   
    mov al, 'l'
    stosw 
    mov al, 'e'
    stosw
    mov al, ':'
    stosw              
endm

invite_string macro 
    mov di, 1120
    mov ah, 00000010b
    mov al, 'I'
    stosw
    mov al, 'n'
    stosw
    mov al, 'p'
    stosw
    mov al, 'u'
    stosw
    mov al, 't'
    stosw
    mov al, ' '
    stosw
    mov al, 'y'
    stosw
    mov al, 'o'
    stosw
    mov al, 'u'
    stosw
    mov al, 'r'
    stosw
    mov al, ' '
    stosw
    mov al, 's'
    stosw
    mov al, 't'
    stosw
    mov al, 'r'
    stosw   
    mov al, 'i'
    stosw 
    mov al, 'n'
    stosw
    mov al, 'g'
    stosw  
    mov al, ':'
    stosw              
endm

return_string macro 
    mov di, 3120
    mov ah, 00000010b
    mov al, 'P'
    stosw
    mov al, 'r'
    stosw
    mov al, 'e'
    stosw
    mov al, 's'
    stosw
    mov al, 's'
    stosw
    mov al, ' '
    stosw
    mov al, 'e'
    stosw
    mov al, 'n'
    stosw
    mov al, 't'
    stosw
    mov al, 'e'
    stosw
    mov al, 'r'
    stosw
    mov al, ' '
    stosw
    mov al, 't'
    stosw
    mov al, 'o'
    stosw   
    mov al, ' '
    stosw 
    mov al, 'r'
    stosw
    mov al, 'e'
    stosw  
    mov al, 't'
    stosw  
    mov al, 'u'
    stosw  
    mov al, 'r'
    stosw  
    mov al, 'n'
    stosw  
    mov al, ' '
    stosw 
    mov al, 't'
    stosw 
    mov al, 'o'
    stosw 
    mov al, ' '
    stosw 
    mov al, 'r'
    stosw              
    mov al, 'e'
    stosw 
    mov al, 'a'
    stosw 
    mov al, 'l'
    stosw
    mov al, ' '
    stosw
    mov al, 'm'
    stosw
    mov al, 'o'
    stosw
    mov al, 'd'
    stosw
    mov al, 'e'
    stosw
    mov al, '!'
    stosw
endm
