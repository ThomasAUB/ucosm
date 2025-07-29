# ARM64 context switching implementation
# Calling convention: X0 = from, X1 = to

.text
.globl contextSwitch
.type contextSwitch, %function

contextSwitch:
    # Save current context (from)
    stp x19, x20, [x0, #0]   # Save X19, X20
    stp x21, x22, [x0, #16]  # Save X21, X22
    stp x23, x24, [x0, #32]  # Save X23, X24
    stp x25, x26, [x0, #48]  # Save X25, X26
    stp x27, x28, [x0, #64]  # Save X27, X28
    stp x29, x30, [x0, #80]  # Save X29 (FP), X30 (LR)
    
    # Save stack pointer
    mov x9, sp
    str x9, [x0, #96]        # Save SP

    # Load target context (to)
    ldp x19, x20, [x1, #0]   # Restore X19, X20
    ldp x21, x22, [x1, #16]  # Restore X21, X22
    ldp x23, x24, [x1, #32]  # Restore X23, X24
    ldp x25, x26, [x1, #48]  # Restore X25, X26
    ldp x27, x28, [x1, #64]  # Restore X27, X28
    ldp x29, x30, [x1, #80]  # Restore X29 (FP), X30 (LR)
    
    # Restore stack pointer
    ldr x9, [x1, #96]        # Restore SP
    mov sp, x9

    ret

.size contextSwitch, .-contextSwitch