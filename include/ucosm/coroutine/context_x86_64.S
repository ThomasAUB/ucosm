# x86_64 context switching implementation
# Calling convention: RDI = from, RSI = to

.text
.globl contextSwitch
.type contextSwitch, @function

contextSwitch:
    # Save current context (from)
    movq %rbx, 0(%rdi)   # Save RBX
    movq %rbp, 8(%rdi)   # Save RBP
    movq %rsp, 16(%rdi)  # Save RSP
    movq %r12, 24(%rdi)  # Save R12
    movq %r13, 32(%rdi)  # Save R13
    movq %r14, 40(%rdi)  # Save R14
    movq %r15, 48(%rdi)  # Save R15

    # Load target context (to)
    movq 0(%rsi), %rbx   # Restore RBX
    movq 8(%rsi), %rbp   # Restore RBP
    movq 16(%rsi), %rsp  # Restore RSP
    movq 24(%rsi), %r12  # Restore R12
    movq 32(%rsi), %r13  # Restore R13
    movq 40(%rsi), %r14  # Restore R14
    movq 48(%rsi), %r15  # Restore R15

    ret

.size contextSwitch, .-contextSwitch