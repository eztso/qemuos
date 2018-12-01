
    // contextSwitch(long* saveArea, long* restoreArea)
    .global contextSwitch
contextSwitch:
    mov 4(%esp),%eax
    mov 8(%esp),%ecx

    mov %cr2,%edx
    push %edx
    pushf

    mov %ebx,0(%eax)
    mov %esp,4(%eax)
    mov %ebp,8(%eax)
    mov %esi,12(%eax)
    mov %edi,16(%eax)

    mov 0(%ecx),%ebx
    cli
    mov 4(%ecx),%esp
    popf
    mov 8(%ecx),%ebp
    mov 12(%ecx),%esi
    mov 16(%ecx),%edi

    movl $0,20(%eax)     # prev->leaveMeAlone = 0
    movl $0,20(%ecx)     # curr->leaveMeAlone = 0

    pop %edx
    mov %edx,%cr2

    ret
