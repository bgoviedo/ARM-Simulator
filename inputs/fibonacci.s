   mov X10, 10
   mov X1, 2
   mov X2, 3
L1:
   add X3, X1, X2
   add X1, X2, 0
   add X2, X3, 0
   add X10, X10, -1
   cmp X10, 0
   bne L1
   hlt 0
