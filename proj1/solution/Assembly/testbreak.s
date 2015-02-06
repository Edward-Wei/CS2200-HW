main:       addi $a0, $zero, 3
			addi $a1, $zero, 3
			addi $a2, $zero, 4
			beq $a2, $a0, darn
			add $v0, $a0, $a1
			halt

darn:
			add $v0, $a0, $a2
             
            halt