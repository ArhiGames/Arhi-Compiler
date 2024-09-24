section .data
section .bss
section .text
 global _start
equals:
 push rbp
 mov rbp, rsp
 sub rsp, 16
 mov qword [rbp-8], rdi
 mov qword [rbp-16], rsi
 mov rax, [rbp-8]
 mov qword rcx, rax
 mov rax, [rbp-16]
 mov qword rdx, rax
 cmp rcx, rdx
 sete al
 mov al, al
 mov rsp, rbp
 pop rbp
 ret
 mov rsp, rbp
 pop rbp
 ret
_start:
 push rbp
 mov rbp, rsp
 mov rsp, rbp
 pop rbp
