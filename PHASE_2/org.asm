.text
addi x1, x0, 5       # x1 = 5
addi x2, x0, 10      # x2 = 10
add x3, x1, x2       # Add x1 and x2, store result in x3
sw x3, 0(x4)         # Store result of addition (x3) into address in x4
addi x4, x0, 20      # x4 = 20
lw x5, 0(x4)         # Load word from address in x4
