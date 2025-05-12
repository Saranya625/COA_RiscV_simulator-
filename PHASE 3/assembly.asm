.data
arr: .word 10, 20, 30, 50, 60, 70, 80,90 # array of 8 elements
arr2: .word 11,22,33
.text
   la x1,arr
   la x2,arr2
   lw x3,20(x1)
   lw x4,4(x2)
   lw x5,0(x1)
   lw x6,0(x2)

   
