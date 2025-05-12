.data
arr1: .word 10,20,30,40,50,60,70,80
arr2: .word 11,22,33,44 
.text
la x1,arr1
la x2,arr2
lw x3,0(x1)
lw x4,0(x2)
lw x5,16(x1)
lw x6,8(x1)