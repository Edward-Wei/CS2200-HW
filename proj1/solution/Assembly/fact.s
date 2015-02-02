!============================================================
! CS-2200 Homework 1
!
! Please do not change main's functionality, 
! except to change the argument for factorial or to meet your 
! calling convention
!============================================================

!factorial(i)
!if i <= 1
!	return 1
!else
!	return i * factorial(i - 1)

main:       la $sp, stack		! load address of stack label into $sp
            noop                        ! FIXME: load desired value of the stack 
                                        ! (defined at the label below) into $sp
            la $at, factorial	        ! load address of factorial label into $at
            addi $a0, $zero, 5          ! $a0 = 5, the number to factorialize
            jalr $at, $ra		  		! jump to factorial, set $ra to return addr
            halt						! when we return, just halt

addOne:		addi $a0, $a0, 1			! fact(0) = 1
			beq $zero, $zero, eval 		! proceed as normal

factorial:  addi $t0, $zero, 1			! Make a "one" register
			beq $t0, $t2, continue		! fp already set
			add $fp, $sp, $zero 		! set fp to sp
			sw $ra, 0($fp)				! push ra
			beq $zero, $a0, addOne		! Check for a zero

continue:	beq $t0, $a0, eval 	 		! Check for eval 
			sw $ra, 0($fp)				! push ra
			sw $a0, 1($fp)				! push n
			addi $a0, $a0, -1			! decrease n
			addi $fp, $fp, 2 			! increment fp
			addi $t2, $zero, 1			! set t2 to 1
			jalr $at, $ra				! recursive call

eval:	  	beq $fp, $sp, finish		! must be finished
			addi $fp, $fp, -2 			! decrment fp
			lw $a1, 1($fp)				! get fp value
			la $t0, mult 				! load mult
 			jalr $t0, $ra 				! call mult
			add $a0, $v0, $zero 		! set return
			la $t0, eval	  			! load eval
			jalr $t0, $ra				! call eval

finish:		add $v0, $a0, $zero  		! set return value
			lw $ra, 0($sp)				! get original return addr
			jalr $ra, $t0				! return to main

mult:		add $v0, $a0, $zero 		! save first arg
 			add $t1, $a1, $zero 		! save other arg
			addi $t1, $t1, -1			! decrement other var

loop:		beq $t1, $zero, endLoop		! finish if needed
			addi $t1, $t1, -1			! decrement other var
			add $v0, $a0, $v0			! do first part of mult
			beq $zero, $zero, loop		! loop until complete

endLoop:	jalr $ra, $t0

stack:	    .word 0x4000		! the stack begins here (for example, that is)

