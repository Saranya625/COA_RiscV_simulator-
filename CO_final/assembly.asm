.data
arr: .word 5, 3   # Array to sort
n:   .word 2                     # Array size

.text
main:
    la x10, arr       # Load base address of array into x10 (a0)
    lw x11, n         # Load array size into x11 (a1)
    addi x11, x11, -1 # n-1 for outer loop condition

    li x12, 0         # i = 0 (Outer loop counter)

outer_loop:
    bge x12, x11, exit # if i >= n-1, exit
    li x13, 0         # j = 0 (Inner loop counter)

inner_loop:
    sub x16, x11, x12  # x16 = n - i - 1 (Loop bound)
    bge x13, x16, next_outer  # if j >= n-i-1, go to next outer iteration

    slli x14, x13, 2    # x14 = j * 4 (Word offset)
    add x14, x10, x14   # x14 = base + j * 4 (Address of arr[j])
    
    lw x15, 0(x14)      # x15 = arr[j]
    lw x16, 4(x14)      # x16 = arr[j+1]

    bge x15, x16, skip_swap  # if arr[j] >= arr[j+1], no swap

    # Swap arr[j] and arr[j+1]
    sw x16, 0(x14)      # arr[j] = arr[j+1]
    sw x15, 4(x14)      # arr[j+1] = arr[j]

skip_swap:
    addi x13, x13, 1   # j++
    j inner_loop       # Repeat inner loop

next_outer:
    addi x12, x12, 1   # i++
    j outer_loop       # Repeat outer loop

exit:
    li x17, 10         # Load exit syscall number
    ecall              # Exit program
