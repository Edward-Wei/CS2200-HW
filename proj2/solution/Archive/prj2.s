!=================================================================
! General conventions:
!   1) Stack grows from high addresses to low addresses, and the
!      top of the stack points to valid data
!
!   2) Register usage is as implied by assembler names and manual
!
!   3) Function Calling Convention:
!
!       Setup)
!       * Immediately upon entering a function, push the RA on the stack.
!       * Next, push all the registers used by the function on the stack.
!
!       Teardown)
!       * Load the return value in $v0.
!       * Pop any saved registers from the stack back into the registers.
!       * Pop the RA back into $ra.
!       * Return by executing jalr $ra, $zero.
!=================================================================

!vector table
vector0:    .fill 0x00000000 !0
            .fill 0x00000000 !1
            .fill 0x00000000 !2
            .fill 0x00000000
            .fill 0x00000000 !4
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000 !8
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000 !15
!end vector table

main:           la $sp, stack           ! Initialize stack pointer
                lw $sp, 0($sp)          
                
                ! Install timer interrupt handler into vector table
                la $ra, vector0
                la $at, ti_inthandler
                sw $at, 1($ra)

                ei                      ! Don't forget to enable interrupts...

                la $at, factorial       ! load address of factorial label into $at
                addi $a0, $zero, 7      ! $a0 = 5, the number to factorialize
                jalr $at, $ra           ! jump to factorial, set $ra to return addr
                halt                    ! when we return, just halt

factorial:      addi    $sp, $sp, -1    ! push RA
                sw      $ra, 0($sp)
                addi    $sp, $sp, -1    ! push a0
                sw      $a0, 0($sp)
                addi    $sp, $sp, -1    ! push s0
                sw      $s0, 0($sp)
                addi    $sp, $sp, -1    ! push s1
                sw      $s1, 0($sp)

                beq     $a0, $zero, base_zero
                addi    $s1, $zero, 1
                beq     $a0, $s1, base_one
                beq     $zero, $zero, recurse
                
    base_zero:  addi    $v0, $zero, 1   ! 0! = 1
                beq     $zero, $zero, done

    base_one:   addi    $v0, $zero, 1   ! 1! = 1
                beq     $zero, $zero, done

    recurse:    add     $s1, $a0, $zero     ! save n in s1
                addi    $a0, $a0, -1        ! n! = n * (n-1)!
                la      $at, factorial
                jalr    $at, $ra

                add     $s0, $v0, $zero     ! use s0 to store (n-1)!
                add     $v0, $zero, $zero   ! use v0 as sum register
        mul:    beq     $s1, $zero, done    ! use s1 as counter (from n to 0)
                add     $v0, $v0, $s0
                addi    $s1, $s1, -1
                beq     $zero, $zero, mul

    done:       lw      $s1, 0($sp)     ! pop s1
                addi    $sp, $sp, 1
                lw      $s0, 0($sp)     ! pop s0
                addi    $sp, $sp, 1
                lw      $a0, 0($sp)     ! pop a0
                addi    $sp, $sp, 1
                lw      $ra, 0($sp)     ! pop RA
                addi    $sp, $sp, 1
                jalr    $ra, $zero

ti_inthandler:  addi $sp, $sp, -14 ! space for registers
                sw $at, 0($sp) ! save all registers
                sw $v0, 1($sp)
                sw $a0, 2($sp)
                sw $a1, 3($sp)
                sw $a2, 4($sp)
                sw $a3, 5($sp)
                sw $a4, 6($sp)
                sw $s0, 7($sp)
                sw $s1, 8($sp)
                sw $s2, 9($sp)
                sw $s3, 10($sp)
                sw $k0, 11($sp)
                sw $fp, 12($sp)
                sw $ra, 13($sp)
                ei

                !store constants
                addi $a0, $zero, 24
                addi $a1, $zero, 60

                la $s0, seconds
                la $s1, minutes
                la $s2, hours

                lw $s0, 0($s0)
                lw $a2, 0($s0)

                lw $s1, 0($s1)
                lw $a3, 0($s1)

                lw $s2, 0($s2)
                lw $a4, 0($s2)

                addi $a2, $a2, 1

                beq $a1, $a2, reset_seconds
                beq $zero, $zero, wrap_up

cont_seconds:   addi $a3, $a3, 1
                beq $a1, $a3, reset_minutes
                beq $zero, $zero, wrap_up

cont_minutes:   addi $a4, $a4, 1
                beq $a0, $a4, reset_hours

wrap_up:     sw $a2, 0($s0) ! save times
                sw $a3, 0($s1)
                sw $a4, 0($s2)

                lw $at, 0($sp) ! save registers
                lw $v0, 1($sp)
                lw $a0, 2($sp)
                lw $a1, 3($sp)
                lw $a2, 4($sp)
                lw $a3, 5($sp)
                lw $a4, 6($sp)
                lw $s0, 7($sp)
                lw $s1, 8($sp)
                lw $s2, 9($sp)
                lw $s3, 10($sp)
                lw $k0, 11($sp)
                lw $fp, 12($sp)
                lw $ra, 13($sp)
                addi $sp, $sp, 14 ! return sp
                reti

reset_hours:
                add $a4, $zero, $zero !zero out
                beq $zero, $zero, wrap_up

reset_minutes:
                add $a3, $zero, $zero !zero out
                beq $zero, $zero, cont_minutes

reset_seconds:
                add $a2, $zero, $zero ! zero out
                beq $zero, $zero, cont_seconds


stack:      .fill 0xA00000
seconds:    .fill 0xFFFFFC
minutes:    .fill 0xFFFFFD
hours:      .fill 0xFFFFFE
