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
fact:
  addi $sp, $sp, -4
  sw $fp, 0($sp)
  move $fp, $sp
  addi $sp, $sp, -4
  move $t0, $a0
  addi $sp, $sp, -4
  move $t1, $t0
  addi $sp, $sp, -4
  li $t3, 1
  move $t2, $t3
  sw $t0, -4($fp)
  beq $t1, $t2, label1
  sw $t1, -8($fp)
  sw $t2, -12($fp)
  j label2
label1:
  addi $sp, $sp, -4
  lw $t1, -4($fp)
  move $t0, $t1
  move $v0, $t0
  addi $sp, $fp, 4
  lw $fp, 0($fp)
  jr $ra
  sw $t0, -16($fp)
  sw $t1, -4($fp)
  j label3
label2:
  addi $sp, $sp, -4
  lw $t1, -4($fp)
  move $t0, $t1
  addi $sp, $sp, -4
  move $t2, $t1
  addi $sp, $sp, -4
  li $t4, 1
  move $t3, $t4
  addi $sp, $sp, -4
  sub $t5, $t2, $t3
  move $a0, $t5
  sw $t0, -4($fp)
  sw $t1, -4($fp)
  sw $t2, -8($fp)
  sw $t3, -12($fp)
  sw $t5, -16($fp)
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal fact
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  addi $sp, $sp, -4
  move $t0, $v0
  addi $sp, $sp, -4
  lw $t2, -4($fp)
  mul $t1, $t2, $t0
  move $v0, $t1
  addi $sp, $fp, 4
  lw $fp, 0($fp)
  jr $ra
  sw $t0, -20($fp)
  sw $t1, -24($fp)
  sw $t2, -4($fp)
label3:
main:
  addi $sp, $sp, -4
  sw $fp, 0($sp)
  move $fp, $sp
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal read
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  addi $sp, $sp, -4
  move $t0, $v0
  addi $sp, $sp, -4
  move $t1, $t0
  addi $sp, $sp, -4
  move $t2, $t1
  addi $sp, $sp, -4
  li $t4, 1
  move $t3, $t4
  sw $t0, -4($fp)
  sw $t1, -8($fp)
  bgt $t2, $t3, label4
  sw $t2, -12($fp)
  sw $t3, -16($fp)
  j label5
label4:
  addi $sp, $sp, -4
  lw $t1, -8($fp)
  move $t0, $t1
  move $a0, $t0
  sw $t0, -20($fp)
  sw $t1, -8($fp)
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal fact
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  addi $sp, $sp, -4
  move $t0, $v0
  addi $sp, $sp, -4
  move $t1, $t0
  sw $t0, -24($fp)
  sw $t1, -28($fp)
  j label6
label5:
  lw $t0, -28($fp)
  li $t1, 1
  move $t0, $t1
  sw $t0, -28($fp)
label6:
  addi $sp, $sp, -4
  lw $t1, -28($fp)
  move $t0, $t1
  move $a0, $t0
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  addi $sp, $sp, -4
  li $t3, 0
  move $t2, $t3
  move $v0, $t2
  addi $sp, $fp, 4
  lw $fp, 0($fp)
  jr $ra
