.data
a:      .word 1,2,3,4,5,6,7,8,9,10,11,12  # Array of 100 elements
sum:    .word 0, 0, 0, 0             # Partial sums for each core (4 words)

.text
main:
    la x5, a          # Load base address of array  
    addi x1,x0,12       # Total elements
    addi x2,x0, 3        # Chunk size per core
    addi x3,x31, 0      # Get core ID
    mul x4, x3 , x2  # Start index for this core  
    slli x4, x4, 2   
    add x5, x5, x4    # Compute start address
    li x6, 0          # sum = 0
    li x7, 3        # Loop counter

loop:
    lw x8, 0(x5)      # Load a[i]
    add x6, x6, x8    # sum += a[i]
    addi x5, x5, 4    # Move to next element
    addi x7, x7, -1   # Decrement counter
    bne x7,x0, loop     # Repeat for 25 elements

    # Store partial sum in memory
    la x9, sum        # Load sum base address
    slli x10, x3, 2   # Offset = core_id * 4
    add x9, x9, x10   # Compute storage location
    sw x6, 0(x9)      # Store partial sum


    la x9, sum
    li x6, 0
    li x7, 4          # 4 cores

sum_loop:
    lw x8, 0(x9)      # Load partial sum
    add x6, x6, x8    # Add to final sum
    addi x9, x9, 4    # Move to next core’s sum
    addi x7, x7, -1   # Decrement counter
    bne x7,x0, sum_loop # Repeat for all cores

skip_print:
    ecall             # Exit program
