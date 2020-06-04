.data
_prompt: .asciiz "Enter an integer:"
_ret: .asciiz "\n"
.globl main
.text
read:
  li $v0, 4
  la $a0, _prompt
  syscall
  li $v0, 5
  syscall
  jr $ra
write:
  li $v0, 1
  syscall
  li $v0, 4
  la $a0, _ret
  syscall
  move $v0, $s0
  jr $ra
main:
  addi $sp, $sp, -4
  sw $fp, 0($sp)
  move $fp, $sp
  addi $sp, $sp, -20
  addi $t0, $fp, -20
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal read
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  addi $sp, $sp, -4
  move $t1, $v0
  addi $sp, $sp, -4
  move $t2, $t1
  addi $sp, $sp, -4
  li $t4, 3
  move $t3, $t4
  addi $sp, $sp, -4
  li $t6, 4
  mul $t5, $t3, $t6
  addi $sp, $sp, -4
  add $t7, $t0, $t5
  sw $t2, 0($t7)
  addi $sp, $sp, -4
  li $t4, 3
  move $t6, $t4
  addi $sp, $sp, -4
  sw $t1, -24($fp)
  sw $t2, -28($fp)
  sw $t3, -32($fp)
  sw $t5, -36($fp)
  sw $t7, -40($fp)
  li $t7, 4
  mul $t4, $t6, $t7
  addi $sp, $sp, -4
  add $t1, $t0, $t4
  addi $sp, $sp, -4
  lw $t2, 0($t1)
  addi $sp, $sp, -4
  li $t5, 1
  move $t3, $t5
  addi $sp, $sp, -4
  add $t7, $t2, $t3
  addi $sp, $sp, -4
  sw $t1, -52($fp)
  sw $t2, -56($fp)
  sw $t3, -60($fp)
  sw $t4, -48($fp)
  sw $t6, -44($fp)
  li $t6, 4
  move $t5, $t6
  addi $sp, $sp, -4
  mul $t1, $t5, $t6
  addi $sp, $sp, -4
  add $t2, $t0, $t1
  sw $t7, 0($t2)
  addi $sp, $sp, -4
  move $t3, $t6
  addi $sp, $sp, -4
  mul $t4, $t3, $t6
  addi $sp, $sp, -4
  add $t6, $t0, $t4
  sw $t1, -72($fp)
  sw $t2, -76($fp)
  sw $t3, -80($fp)
  sw $t4, -84($fp)
  sw $t5, -68($fp)
  sw $t7, -64($fp)
  addi $sp, $sp, -4
  lw $t7, 0($t6)
  move $a0, $t7
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  addi $sp, $sp, -4
  li $t1, 0
  move $t0, $t1
  move $v0, $t0
  addi $sp, $fp, 4
  lw $fp, 0($fp)
  jr $ra
