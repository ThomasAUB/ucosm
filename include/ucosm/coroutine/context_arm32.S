# ARM32 context switching implementation
# Calling convention: R0 = from, R1 = to

.text
.globl contextSwitch
.type contextSwitch, %function

contextSwitch:
    # Save current context (from)
    stmia r0!, {r4-r11}      # Save R4-R11
    str sp, [r0, #0]         # Save SP
    str lr, [r0, #4]         # Save LR

    # Load target context (to)
    ldmia r1!, {r4-r11}      # Restore R4-R11
    ldr sp, [r1, #0]         # Restore SP
    ldr lr, [r1, #4]         # Restore LR

    bx lr                    # Return

.size contextSwitch, .-contextSwitch