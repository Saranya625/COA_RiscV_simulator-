.data
a:      .word 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20  # Array of 100 elements
sum:    .word 0          # Partial sums for each core (4 words)
final_sum: .word 0
.text
main:
    la x5, a          # Load base address of array  
    addi x1,x0,20       # Total elements
    addi x2,x0, 5        # Chunk size per core
    addi x3,x31, 0      # Get core ID
    mul x4, x3 , x2  # Start index for this core  
    slli x4, x4, 2   
    add x5, x5, x4    # Compute start address
    li x6, 0          # sum = 0
    li x7, 5        # Loop counter

loop:
    lw x8, 0(x5)      # Load a[i]
    add x6, x6, x8    # sum += a[i]
    addi x5, x5, 4    # Move to next element
    addi x7, x7, -1   # Decrement counter
    bne x7,x0, loop     # Repeat for 25 elements
    la x9, sum     
    sw x6, 0(x9)    # Load sum base address4  
    li x6, 0
    li x7, 4        # Store partial sum 
    bne cid , 1,exit
    bne cid,2 ,exit
    bne cid , 3, exit 
sum_loop:
 lw x8,0(x9)
 addi x9,x9,1024
 add x11,x11,x8
 addi x7,x7,-1
 bne x7,x0,sum_loop
 la x12,final_sum
 sw x11,0(x12) 
exit:
ecall


