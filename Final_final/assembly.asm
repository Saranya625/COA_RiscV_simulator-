.data
arr: .word 64, 34, 25, 12, 22, 11, 90

.text
main:
    la x1, arr        # Load base address of array
    addi x2,x0, 7          # Load array size into x2
    addi x2, x2, -1   # n-1 for outer loop
    li x3, 0          # i = 0

outer_loop:
    beq x3, x2, end   # if i == n-1, end sorting
    sub x4, x2, x3    # x4 = n-1-i
    li x5, 0          # j = 0

inner_loop:
    beq x5, x4, next_outer  # if j == n-1-i, continue outer loop
    slli x6, x5, 2          # x6 = j * 4 (word offset)
    add x6, x1, x6          # x6 = &arr[j]
    lw x7, 0(x6)            # x7 = arr[j]
    lw x8, 4(x6)            # x8 = arr[j+1]

    bne x7, x8, check_swap  # If arr[j] != arr[j+1], check swap

check_swap:
    blt x7, x8, skip_swap   # If arr[j] <= arr[j+1], skip swap
    sw x8, 0(x6)            # Swap arr[j] and arr[j+1]
    sw x7, 4(x6)            # Swap arr[j] and arr[j+1]

skip_swap:
    addi x5, x5, 1          # j++
    j inner_loop            # Continue inner loop

next_outer:
    addi x3, x3, 1          # i++
    j outer_loop            # Continue outer loop

end:
    ecall
#bubble_sort code with functions written in the simulator 

