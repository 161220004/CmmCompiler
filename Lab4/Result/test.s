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
  addi $sp, $sp, -4
  li $t1, 0
  move $t0, $t1
  addi $sp, $sp, -4
  li $t3, 1
  move $t2, $t3
  addi $sp, $sp, -4
  move $t4, $t1
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal read
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  addi $sp, $sp, -4
  move $t5, $v0
  addi $sp, $sp, -4
  move $t6, $t5
  sw $t0, -4($fp)
  sw $t2, -8($fp)
  sw $t4, -12($fp)
  sw $t5, -16($fp)
  sw $t6, -20($fp)
label1:
  addi $sp, $sp, -4
  lw $t1, -12($fp)
  move $t0, $t1
  addi $sp, $sp, -4
  lw $t3, -20($fp)
  move $t2, $t3
  sw $t1, -12($fp)
  sw $t3, -20($fp)
  blt $t0, $t2, label2
  sw $t0, -24($fp)
  sw $t2, -28($fp)
  j label3
label2:
  addi $sp, $sp, -4
  lw $t1, -4($fp)
  move $t0, $t1
  addi $sp, $sp, -4
  lw $t3, -8($fp)
  move $t2, $t3
  addi $sp, $sp, -4
  add $t4, $t0, $t2
  addi $sp, $sp, -4
  move $t5, $t4
  addi $sp, $sp, -4
  move $t6, $t3
  move $a0, $t6
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  move $t1, $t3
  move $t3, $t5
  addi $sp, $sp, -4
  sw $t0, -32($fp)
  sw $t1, -4($fp)
  sw $t2, -36($fp)
  sw $t3, -8($fp)
  sw $t4, -40($fp)
  sw $t5, -44($fp)
  sw $t6, -48($fp)
  lw $t6, -12($fp)
  move $t7, $t6
  addi $sp, $sp, -4
  li $t1, 1
  move $t0, $t1
  addi $sp, $sp, -4
  add $t2, $t7, $t0
  move $t6, $t2
  sw $t0, -56($fp)
  sw $t2, -60($fp)
  sw $t6, -12($fp)
  sw $t7, -52($fp)
  j label1
label3:
  addi $sp, $sp, -4
  li $t1, 0
  move $t0, $t1
  move $v0, $t0
  addi $sp, $fp, 4
  lw $fp, 0($fp)
  jr $ra
