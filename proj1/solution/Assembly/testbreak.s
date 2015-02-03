main:       addi $a0, $zero, 3
			addi $a1, $zero, 5
			addi $a2, $zero, 3
			beq $a0, $a2, darn
			add $v0, $a0, $a1
			halt

darn:
			add $v0, $a0, $a2
             
            halt