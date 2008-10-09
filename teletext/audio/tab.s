	.data
	.globl	camlTab__data_begin
camlTab__data_begin:
	.text
	.globl	camlTab__code_begin
camlTab__code_begin:
	.data
	.long	2048
	.globl	camlTab
camlTab:
	.space	8
	.data
	.long	2295
camlTab__1:
	.long	camlTab__tab_60
	.long	3
	.data
	.long	2295
camlTab__2:
	.long	camlTab__fun_79
	.long	3
	.data
	.long	2048
camlTab__3:
	.long	1
	.long	.L100007
	.long	2048
.L100007:
	.long	5
	.long	.L100008
	.long	2048
.L100008:
	.long	9
	.long	.L100009
	.long	2048
.L100009:
	.long	13
	.long	.L100010
	.long	2048
.L100010:
	.long	17
	.long	.L100011
	.long	2048
.L100011:
	.long	21
	.long	.L100012
	.long	2048
.L100012:
	.long	25
	.long	.L100013
	.long	2048
.L100013:
	.long	29
	.long	.L100014
	.long	2048
.L100014:
	.long	33
	.long	.L100015
	.long	2048
.L100015:
	.long	37
	.long	.L100016
	.long	2048
.L100016:
	.long	41
	.long	.L100017
	.long	2048
.L100017:
	.long	45
	.long	.L100018
	.long	2048
.L100018:
	.long	49
	.long	.L100019
	.long	2048
.L100019:
	.long	53
	.long	.L100020
	.long	2048
.L100020:
	.long	57
	.long	1
	.data
	.long	2048
camlTab__4:
	.long	.L100006
	.long	1
	.long	2301
.L100006:
	.double	0.0
	.data
	.long	6396
camlTab__5:
	.ascii	"%d, %f, %f, %f, %f, %f\12"
	.byte	0
	.text
	.align	16
	.globl	camlTab__fun_79
	.type	camlTab__fun_79,@function
camlTab__fun_79:
	subl	$8, %esp
.L100:
	fldl	.L101
	sarl	$1, %eax
	pushl	%eax
	fildl	(%esp)
	addl	$4, %esp
	fdivp	%st, %st(1)
	subl	$8, %esp
	fstpl	0(%esp)
	fldl	.L102
	subl	$8, %esp
	fstpl	0(%esp)
	call	pow
	addl	$16, %esp
	fstpl	0(%esp)
	fldl	.L103
	fmull	0(%esp)
	fstpl	0(%esp)
	fld1
	fdivl	0(%esp)
	fstpl	0(%esp)
.L104:	movl	caml_young_ptr, %eax
	subl	$12, %eax
	movl	%eax, caml_young_ptr
	cmpl	caml_young_limit, %eax
	jb	.L105
	leal	4(%eax), %eax
	movl	$2301, -4(%eax)
	fldl	0(%esp)
	fstpl	(%eax)
	addl	$8, %esp
	ret
.L105:	call	caml_call_gc
.L106:	jmp	.L104
	.data
.L103:	.double	3.
	.data
.L102:	.double	10.
	.data
.L101:	.double	20.
	.text
	.align	16
	.globl	camlTab__tab_60
	.type	camlTab__tab_60,@function
camlTab__tab_60:
	subl	$56, %esp
.L116:
	movl	$1, %eax
	cmpl	$511, %eax
	jg	.L107
	movl	%eax, 16(%esp)
.L108:
	fldl	.L117
	sarl	$1, %eax
	pushl	%eax
	fildl	(%esp)
	addl	$4, %esp
	fdivp	%st, %st(1)
	fstpl	48(%esp)
.L118:	movl	caml_young_ptr, %eax
	subl	$52, %eax
	movl	%eax, caml_young_ptr
	cmpl	caml_young_limit, %eax
	jb	.L119
	leal	4(%eax), %ebx
	movl	%ebx, 8(%esp)
	movl	$2301, -4(%ebx)
	fldl	48(%esp)
	fstpl	(%ebx)
	movl	camlPervasives + 36, %eax
	movl	%eax, 12(%esp)
	leal	12(%ebx), %edx
	movl	$1024, -4(%edx)
	movl	$33, (%edx)
	leal	20(%ebx), %ecx
	movl	$1024, -4(%ecx)
	movl	$33, (%ecx)
	leal	28(%ebx), %eax
	movl	$1024, -4(%eax)
	movl	$33, (%eax)
	addl	$36, %ebx
	movl	$3072, -4(%ebx)
	movl	%eax, (%ebx)
	movl	%ecx, 4(%ebx)
	movl	%edx, 8(%ebx)
	movl	8(%ebx), %eax
	movl	%eax, 20(%esp)
	movl	4(%ebx), %ebp
	movl	%ebp, 0(%esp)
	movl	(%ebx), %edi
	movl	%edi, 4(%esp)
	movl	$1, %esi
	cmpl	$31, %esi
	jg	.L109
.L110:
	movl	$1, %edx
	cmpl	$31, %edx
	jg	.L111
.L112:
	movl	$1, %ecx
	cmpl	$31, %ecx
	jg	.L113
.L114:
	movl	camlTab, %eax
	movl	-4(%eax), %ebx
	shrl	$10, %ebx
	cmpl	%esi, %ebx
	jbe	.L121
	fldl	-4(%eax, %esi, 4)
	fstpl	40(%esp)
	movl	camlTab, %eax
	movl	-4(%eax), %ebx
	shrl	$10, %ebx
	cmpl	%edx, %ebx
	jbe	.L121
	fldl	-4(%eax, %edx, 4)
	fstpl	32(%esp)
	movl	camlTab, %eax
	movl	-4(%eax), %ebx
	shrl	$10, %ebx
	cmpl	%ecx, %ebx
	jbe	.L121
	fldl	-4(%eax, %ecx, 4)
	fstpl	24(%esp)
	fldl	40(%esp)
	faddl	32(%esp)
	faddl	24(%esp)
	fstpl	24(%esp)
	fldl	24(%esp)
	fsubl	48(%esp)
	fabs
	fstpl	24(%esp)
.L122:	movl	caml_young_ptr, %eax
	subl	$12, %eax
	movl	%eax, caml_young_ptr
	cmpl	caml_young_limit, %eax
	jb	.L123
	leal	4(%eax), %ebx
	movl	$2301, -4(%ebx)
	fldl	24(%esp)
	fstpl	(%ebx)
	movl	12(%esp), %eax
	fldl	(%eax)
	fcompl	24(%esp)
	fnstsw	%ax
	andb	$69, %ah
	jne	.L115
	movl	%ebx, %eax
	movl	%eax, 12(%esp)
	movl	%esi, (%edi)
	movl	%edx, (%ebp)
	movl	20(%esp), %eax
	movl	%ecx, (%eax)
.L115:
	movl	%ecx, %eax
	addl	$2, %ecx
	cmpl	$31, %eax
	jne	.L114
.L113:
	movl	%edx, %eax
	addl	$2, %edx
	cmpl	$31, %eax
	jne	.L112
.L111:
	movl	%esi, %eax
	addl	$2, %esi
	cmpl	$31, %eax
	jne	.L110
.L109:
	movl	$camlTab__5, %eax
	call	camlPrintf__printf_364
.L125:
	movl	%eax, 12(%esp)
.L126:	movl	caml_young_ptr, %eax
	subl	$48, %eax
	movl	%eax, caml_young_ptr
	cmpl	caml_young_limit, %eax
	jb	.L127
	leal	4(%eax), %esi
	movl	$2301, -4(%esi)
	movl	0(%esp), %edi
	movl	(%edi), %ecx
	movl	camlTab, %ebx
	movl	-4(%ebx), %eax
	shrl	$10, %eax
	cmpl	%ecx, %eax
	jbe	.L121
	fldl	-4(%ebx, %ecx, 4)
	movl	4(%esp), %ebp
	movl	(%ebp), %ecx
	movl	camlTab, %ebx
	movl	-4(%ebx), %eax
	shrl	$10, %eax
	cmpl	%ecx, %eax
	jbe	.L121
	fldl	-4(%ebx, %ecx, 4)
	faddp	%st, %st(1)
	movl	20(%esp), %ecx
	movl	(%ecx), %ebx
	movl	camlTab, %eax
	movl	-4(%eax), %edx
	shrl	$10, %edx
	cmpl	%ebx, %edx
	jbe	.L121
	fldl	-4(%eax, %ebx, 4)
	faddp	%st, %st(1)
	fstpl	(%esi)
	leal	12(%esi), %edx
	movl	$2301, -4(%edx)
	movl	(%ecx), %ebx
	movl	camlTab, %eax
	movl	-4(%eax), %ecx
	shrl	$10, %ecx
	cmpl	%ebx, %ecx
	jbe	.L121
	fldl	-4(%eax, %ebx, 4)
	fstpl	(%edx)
	leal	24(%esi), %ecx
	movl	$2301, -4(%ecx)
	movl	(%edi), %ebx
	movl	camlTab, %eax
	movl	-4(%eax), %edi
	shrl	$10, %edi
	cmpl	%ebx, %edi
	jbe	.L121
	fldl	-4(%eax, %ebx, 4)
	fstpl	(%ecx)
	leal	36(%esi), %ebx
	movl	$2301, -4(%ebx)
	movl	(%ebp), %edi
	movl	camlTab, %eax
	movl	-4(%eax), %ebp
	shrl	$10, %ebp
	cmpl	%edi, %ebp
	jbe	.L121
	fldl	-4(%eax, %edi, 4)
	fstpl	(%ebx)
	movl	16(%esp), %eax
	movl	8(%esp), %edi
	movl	12(%esp), %ebp
	movl	%ebp, caml_extra_params + 0
	call	caml_apply6
.L129:
	movl	16(%esp), %eax
	movl	%eax, %ebx
	addl	$2, %eax
	movl	%eax, 16(%esp)
	cmpl	$511, %ebx
	jne	.L108
.L107:
	movl	$1, %eax
	addl	$56, %esp
	ret
.L127:	call	caml_call_gc
.L128:	jmp	.L126
.L123:	call	caml_call_gc
.L124:	jmp	.L122
.L119:	call	caml_call_gc
.L120:	jmp	.L118
.L121:	call	caml_ml_array_bound_error
	.data
.L117:	.double	255.
	.text
	.align	16
	.globl	camlTab__entry
	.type	camlTab__entry,@function
camlTab__entry:
.L130:
	movl	$camlTab__3, %ebx
	movl	$camlTab__2, %eax
	call	camlList__map_90
.L131:
	movl	$camlTab__4, %ebx
	call	camlPervasives__$40_167
.L132:
	call	camlArray__of_list_157
.L133:
	movl	%eax, camlTab
	movl	$camlTab__1, %eax
	movl	%eax, camlTab + 4
	movl	$1, %eax
	call	camlTab__tab_60
.L134:
	movl	$1, %eax
	ret
	.text
	.globl	camlTab__code_end
camlTab__code_end:
	.data
	.globl	camlTab__data_end
camlTab__data_end:
	.long	0
	.globl	camlTab__frametable
camlTab__frametable:
	.long	10
	.long	.L134
	.word	4
	.word	0
	.align	4
	.long	.L133
	.word	4
	.word	0
	.align	4
	.long	.L132
	.word	4
	.word	0
	.align	4
	.long	.L131
	.word	4
	.word	0
	.align	4
	.long	.L129
	.word	60
	.word	0
	.align	4
	.long	.L128
	.word	60
	.word	5
	.word	0
	.word	4
	.word	8
	.word	12
	.word	20
	.align	4
	.long	.L125
	.word	60
	.word	4
	.word	0
	.word	4
	.word	8
	.word	20
	.align	4
	.long	.L124
	.word	60
	.word	7
	.word	0
	.word	4
	.word	8
	.word	12
	.word	20
	.word	11
	.word	13
	.align	4
	.long	.L120
	.word	60
	.word	0
	.align	4
	.long	.L106
	.word	12
	.word	0
	.align	4
