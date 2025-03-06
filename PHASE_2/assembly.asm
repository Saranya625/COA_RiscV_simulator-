.text
addi x1, x0, 3       # x1 = 3
addi x2, x0, 5       # x2 = 5
add x3, x1, x2       # Add x1 and x2, result in x3
sw x3, 0(x4)         # Store result of addition (x3) into address in x4
lw x5, 0(x4)         # Load word from address in x4
sub x6, x5, x1       # Subtract x1 from x5, result goes to x6
