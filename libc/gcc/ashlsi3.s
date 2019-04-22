# GCC language support routines
# 32-bit integer arithmetic
# long __ashlsi3 (long a, long b)

	.code16

	.text

	.global	__ashlsi3

__ashlsi3:
	mov %sp,%bx
	mov 2(%bx),%ax
	mov 4(%bx),%dx
	mov 6(%bx),%cx
    jcxz 2f
1:
	shl $1,%ax
	rcl $1,%dx
	loop 1b
2:
	ret
