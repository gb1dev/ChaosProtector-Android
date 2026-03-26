	.text
	.file	"test_vm_debug.c"
	.globl	add                             # -- Begin function add
	.p2align	4, 0x90
	.type	add,@function
add:                                    # @add
	.cfi_startproc
# %bb.0:
	subq	$24, %rsp
	.cfi_def_cfa_offset 32
	movl	%edi, %eax
	movq	%rax, 8(%rsp)
	movl	%esi, %eax
	movq	%rax, 16(%rsp)
	leaq	.L__chaos_bc_add(%rip), %rdi
	leaq	8(%rsp), %rsi
	movl	$2, %edx
	callq	__chaos_vm_interpret@PLT
                                        # kill: def $eax killed $eax killed $rax
	addq	$24, %rsp
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	add, .Lfunc_end0-add
	.cfi_endproc
                                        # -- End function
	.globl	fibonacci                       # -- Begin function fibonacci
	.p2align	4, 0x90
	.type	fibonacci,@function
fibonacci:                              # @fibonacci
	.cfi_startproc
# %bb.0:
	movl	%edi, %eax
	cmpl	$2, %edi
	jl	.LBB1_3
# %bb.1:
	decl	%eax
	movl	$1, %ecx
	xorl	%esi, %esi
	movl	%eax, %edx
	.p2align	4, 0x90
.LBB1_2:                                # =>This Inner Loop Header: Depth=1
	movl	%esi, %eax
	addl	%ecx, %eax
	movl	%ecx, %esi
	movl	%eax, %ecx
	decl	%edx
	jne	.LBB1_2
.LBB1_3:
	retq
.Lfunc_end1:
	.size	fibonacci, .Lfunc_end1-fibonacci
	.cfi_endproc
                                        # -- End function
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	pushq	%r14
	.cfi_def_cfa_offset 24
	pushq	%rbx
	.cfi_def_cfa_offset 32
	subq	$16, %rsp
	.cfi_def_cfa_offset 48
	.cfi_offset %rbx, -32
	.cfi_offset %r14, -24
	.cfi_offset %rbp, -16
	leaq	.Lstr(%rip), %rdi
	callq	puts@PLT
	movq	$3, (%rsp)
	movq	$7, 8(%rsp)
	leaq	.L__chaos_bc_add(%rip), %r14
	movq	%rsp, %rsi
	movq	%r14, %rdi
	movl	$2, %edx
	callq	__chaos_vm_interpret@PLT
	movq	%rax, %rbx
	leaq	.L.str.1(%rip), %rdi
	movl	%ebx, %esi
	xorl	%eax, %eax
	callq	printf@PLT
	leaq	.Lstr.9(%rip), %rdi
	callq	puts@PLT
	xorps	%xmm0, %xmm0
	movaps	%xmm0, (%rsp)
	movq	%rsp, %rsi
	movq	%r14, %rdi
	movl	$2, %edx
	callq	__chaos_vm_interpret@PLT
	movq	%rax, %r14
	leaq	.L.str.3(%rip), %rdi
	movl	%r14d, %esi
	xorl	%eax, %eax
	callq	printf@PLT
	leaq	.Lstr.10(%rip), %rdi
	callq	puts@PLT
	leaq	.L.str.5(%rip), %rdi
	movl	$55, %esi
	xorl	%eax, %eax
	callq	printf@PLT
	leaq	.Lstr.11(%rip), %rdi
	callq	puts@PLT
	leaq	.L.str.7(%rip), %rdi
	xorl	%esi, %esi
	xorl	%eax, %eax
	callq	printf@PLT
	xorl	%ebp, %ebp
	cmpl	$10, %ebx
	sete	%bpl
	cmpl	$1, %r14d
	adcl	$2, %ebp
	leaq	.L.str.8(%rip), %rdi
	movl	%ebp, %esi
	xorl	%eax, %eax
	callq	printf@PLT
	xorl	%eax, %eax
	cmpl	$4, %ebp
	setne	%al
	addq	$16, %rsp
	.cfi_def_cfa_offset 32
	popq	%rbx
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end2:
	.size	main, .Lfunc_end2-main
	.cfi_endproc
                                        # -- End function
	.globl	__chaos_vm_interpret            # -- Begin function __chaos_vm_interpret
	.p2align	4, 0x90
	.type	__chaos_vm_interpret,@function
__chaos_vm_interpret:                   # @__chaos_vm_interpret
# %bb.0:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movzwl	4(%rdi), %eax
	movl	%eax, %ecx
	subl	$513, %ecx                      # imm = 0x201
	movl	12(%rdi), %ecx
	movl	$512, %r8d                      # imm = 0x200
	cmovael	%r8d, %eax
	movl	%ecx, %r8d
	subl	$129, %r8d
	movl	$128, %r8d
	cmovael	%r8d, %ecx
	leal	15(,%rax,8), %eax
	andl	$-16, %eax
	movq	%rsp, %r8
	subq	%rax, %r8
	movq	%r8, %rsp
	leaq	15(,%rcx,8), %rax
	andq	$-16, %rax
	movq	%rsp, %r9
	subq	%rax, %r9
	movq	%r9, %rsp
	movl	$0, -4(%rbp)
	movl	$16, -8(%rbp)
	movq	$0, -24(%rbp)
	movl	$1, -12(%rbp)
	cmpl	$0, %edx
	jle	.LBB3_4
# %bb.1:
	xorl	%eax, %eax
	jmp	.LBB3_2
.LBB3_2:                                # =>This Inner Loop Header: Depth=1
	movslq	%eax, %rcx
	movq	(%rsi,%rcx,8), %rcx
	movslq	%eax, %r10
	movslq	%eax, %r11
	movq	%rcx, (%r8,%r10,8)
	addl	$1, %eax
	cmpl	%edx, %eax
	jl	.LBB3_2
# %bb.3:
	jmp	.LBB3_4
.LBB3_4:
	jmp	.LBB3_5
.LBB3_5:                                # =>This Inner Loop Header: Depth=1
	cmpl	$0, -12(%rbp)
	jne	.LBB3_7
# %bb.6:
	movq	-24(%rbp), %rax
	movq	%rbp, %rsp
	popq	%rbp
	retq
.LBB3_7:                                #   in Loop: Header=BB3_5 Depth=1
	movslq	-8(%rbp), %rax
	movb	(%rdi,%rax), %cl
	incl	%eax
	movl	%eax, -8(%rbp)
	addb	$112, %cl
	movzbl	%cl, %eax
	subb	$-31, %cl
	ja	.LBB3_9
# %bb.8:                                #   in Loop: Header=BB3_5 Depth=1
	leaq	.LJTI3_0(%rip), %rcx
	movslq	(%rcx,%rax,4), %rax
	addq	%rcx, %rax
	jmpq	*%rax
.LBB3_9:                                #   in Loop: Header=BB3_5 Depth=1
	movl	$0, -12(%rbp)
	jmp	.LBB3_11
.LBB3_10:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-8(%rbp), %eax
	movslq	%eax, %rcx
	movl	(%rdi,%rcx), %ecx
	addl	$4, %eax
	movl	%eax, -8(%rbp)
	movslq	%ecx, %rax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
.LBB3_11:                               #   in Loop: Header=BB3_5 Depth=1
	jmp	.LBB3_5
.LBB3_12:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-8(%rbp), %eax
	movslq	%eax, %rcx
	movq	(%rdi,%rcx), %rcx
	addl	$8, %eax
	movl	%eax, -8(%rbp)
	movl	-4(%rbp), %eax
	movslq	%eax, %rdx
	movq	%rcx, (%r9,%rdx,8)
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB3_11
.LBB3_13:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-8(%rbp), %eax
	movslq	%eax, %rcx
	movw	(%rdi,%rcx), %cx
	addl	$2, %eax
	movl	%eax, -8(%rbp)
	movzwl	%cx, %eax
	cltq
	movq	(%r8,%rax,8), %rax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_14:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-8(%rbp), %eax
	movslq	%eax, %rcx
	movw	(%rdi,%rcx), %cx
	addl	$2, %eax
	movl	%eax, -8(%rbp)
	movzwl	%cx, %eax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rdx
	movq	(%r9,%rdx,8), %rdx
	cltq
	movslq	%ecx, %rcx
	movq	%rdx, (%r8,%rax,8)
	jmp	.LBB3_11
.LBB3_15:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-8(%rbp), %eax
	movslq	%eax, %rcx
	movb	(%rdi,%rcx), %cl
	addl	$1, %eax
	movl	%eax, -8(%rbp)
	movzbl	%cl, %eax
	cltq
	movq	(%rsi,%rax,8), %rax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_16:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	addq	(%r9,%rcx,8), %rax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_17:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rdx
	movq	(%r9,%rdx,8), %rdx
	movslq	%ecx, %rcx
	subq	%rax, %rdx
	movl	-4(%rbp), %eax
	movslq	%eax, %rcx
	movq	%rdx, (%r9,%rcx,8)
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB3_11
.LBB3_18:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	imulq	(%r9,%rcx,8), %rax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_19:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rcx
	movl	-4(%rbp), %edx
	subl	$1, %edx
	movl	%edx, -4(%rbp)
	movslq	%edx, %rax
	movq	(%r9,%rax,8), %rax
	movslq	%edx, %rdx
	cqto
	idivq	%rcx
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_20:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rcx
	movl	-4(%rbp), %edx
	subl	$1, %edx
	movl	%edx, -4(%rbp)
	movslq	%edx, %rax
	movq	(%r9,%rax,8), %rax
	movslq	%edx, %rdx
	cqto
	idivq	%rcx
	movl	-4(%rbp), %eax
	movslq	%eax, %rcx
	movq	%rdx, (%r9,%rcx,8)
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB3_11
.LBB3_21:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	andq	(%r9,%rcx,8), %rax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_22:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	orq	(%r9,%rcx,8), %rax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_23:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	xorq	(%r9,%rcx,8), %rax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_24:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rcx
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	movslq	%eax, %rdx
	movq	(%r9,%rdx,8), %rdx
                                        # kill: def $cl killed $rcx
	cltq
	shlq	%cl, %rdx
	movl	-4(%rbp), %eax
	movslq	%eax, %rcx
	movq	%rdx, (%r9,%rcx,8)
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB3_11
.LBB3_25:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rcx
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	movslq	%eax, %rdx
	movq	(%r9,%rdx,8), %rdx
                                        # kill: def $cl killed $rcx
	cltq
	shrq	%cl, %rdx
	movl	-4(%rbp), %eax
	movslq	%eax, %rcx
	movq	%rdx, (%r9,%rcx,8)
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB3_11
.LBB3_26:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rcx
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	movslq	%eax, %rdx
	movq	(%r9,%rdx,8), %rdx
                                        # kill: def $cl killed $rcx
	cltq
	sarq	%cl, %rdx
	movl	-4(%rbp), %eax
	movslq	%eax, %rcx
	movq	%rdx, (%r9,%rcx,8)
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	.LBB3_11
.LBB3_27:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	cmpq	%rax, (%r9,%rcx,8)
	sete	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_28:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	cmpq	%rax, (%r9,%rcx,8)
	setne	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_29:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	cmpq	%rax, (%r9,%rcx,8)
	setl	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_30:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	cmpq	%rax, (%r9,%rcx,8)
	setg	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_31:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	cmpq	%rax, (%r9,%rcx,8)
	setle	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_32:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	cmpq	%rax, (%r9,%rcx,8)
	setge	%al
	andb	$1, %al
	movzbl	%al, %eax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_33:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	(%rax), %eax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_34:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	movslq	%eax, %rcx
	movq	(%r9,%rcx,8), %rcx
	cltq
	movq	(%rcx), %rax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_35:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	movq	(%r9,%rcx,8), %rcx
	movl	%eax, (%rcx)
	jmp	.LBB3_11
.LBB3_36:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rdx
	movq	(%r9,%rdx,8), %rdx
	movslq	%ecx, %rcx
	movq	%rax, (%rdx)
	jmp	.LBB3_11
.LBB3_37:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-8(%rbp), %eax
	movslq	%eax, %rcx
	movl	(%rdi,%rcx), %ecx
	addl	$4, %eax
	movl	%eax, -8(%rbp)
	movl	%ecx, -8(%rbp)
	jmp	.LBB3_11
.LBB3_38:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-8(%rbp), %ecx
	movslq	%ecx, %rax
	movl	(%rdi,%rax), %eax
	addl	$4, %ecx
	movl	%ecx, -8(%rbp)
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	cmpq	$0, (%r9,%rcx,8)
	jne	.LBB3_46
	jmp	.LBB3_47
.LBB3_39:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	movslq	%eax, %rcx
	movq	(%r9,%rcx,8), %rcx
	cltq
	movq	%rcx, -24(%rbp)
	movl	$0, -12(%rbp)
	jmp	.LBB3_11
.LBB3_40:                               #   in Loop: Header=BB3_5 Depth=1
	movq	$0, -24(%rbp)
	movl	$0, -12(%rbp)
	jmp	.LBB3_11
.LBB3_41:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	addq	(%r9,%rcx,8), %rax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_42:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	cltq
	movq	(%r9,%rax,8), %rax
	movl	-4(%rbp), %ecx
	subl	$1, %ecx
	movl	%ecx, -4(%rbp)
	movslq	%ecx, %rcx
	movq	(%r9,%rcx,8), %rcx
	movl	-4(%rbp), %edx
	subl	$1, %edx
	movl	%edx, -4(%rbp)
	movslq	%edx, %rdx
	cmpq	$0, (%r9,%rdx,8)
	cmovneq	%rcx, %rax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_43:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	movslq	%eax, %rcx
	movq	(%r9,%rcx,8), %rcx
	cltq
	movslq	%ecx, %rax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_44:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	movslq	%eax, %rcx
	movq	(%r9,%rcx,8), %rcx
	cltq
	movl	%ecx, %eax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_45:                               #   in Loop: Header=BB3_5 Depth=1
	movl	-4(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	movslq	%eax, %rcx
	movq	(%r9,%rcx,8), %rcx
	cltq
	movl	%ecx, %eax
	movl	-4(%rbp), %ecx
	movslq	%ecx, %rdx
	movq	%rax, (%r9,%rdx,8)
	addl	$1, %ecx
	movl	%ecx, -4(%rbp)
	jmp	.LBB3_11
.LBB3_46:                               #   in Loop: Header=BB3_5 Depth=1
	movl	%eax, -8(%rbp)
	jmp	.LBB3_11
.LBB3_47:                               #   in Loop: Header=BB3_5 Depth=1
	jmp	.LBB3_11
.Lfunc_end3:
	.size	__chaos_vm_interpret, .Lfunc_end3-__chaos_vm_interpret
	.section	.rodata,"a",@progbits
	.p2align	2, 0x0
.LJTI3_0:
	.long	.LBB3_39-.LJTI3_0
	.long	.LBB3_40-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_15-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_41-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_42-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_10-.LJTI3_0
	.long	.LBB3_12-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_13-.LJTI3_0
	.long	.LBB3_14-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_16-.LJTI3_0
	.long	.LBB3_17-.LJTI3_0
	.long	.LBB3_18-.LJTI3_0
	.long	.LBB3_19-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_20-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_21-.LJTI3_0
	.long	.LBB3_22-.LJTI3_0
	.long	.LBB3_23-.LJTI3_0
	.long	.LBB3_24-.LJTI3_0
	.long	.LBB3_25-.LJTI3_0
	.long	.LBB3_26-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_27-.LJTI3_0
	.long	.LBB3_28-.LJTI3_0
	.long	.LBB3_29-.LJTI3_0
	.long	.LBB3_31-.LJTI3_0
	.long	.LBB3_30-.LJTI3_0
	.long	.LBB3_32-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_33-.LJTI3_0
	.long	.LBB3_34-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_35-.LJTI3_0
	.long	.LBB3_36-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_43-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_44-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_45-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_9-.LJTI3_0
	.long	.LBB3_37-.LJTI3_0
	.long	.LBB3_38-.LJTI3_0
                                        # -- End function
	.type	.L.str.1,@object                # @.str.1
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.1:
	.asciz	"add(3,7) = %d (expected 10)\n"
	.size	.L.str.1, 29

	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"add(0,0) = %d (expected 0)\n"
	.size	.L.str.3, 28

	.type	.L.str.5,@object                # @.str.5
.L.str.5:
	.asciz	"fib(10) = %d (expected 55)\n"
	.size	.L.str.5, 28

	.type	.L.str.7,@object                # @.str.7
.L.str.7:
	.asciz	"fib(0) = %d (expected 0)\n"
	.size	.L.str.7, 26

	.type	.L.str.8,@object                # @.str.8
.L.str.8:
	.asciz	"=== %d/4 passed ===\n"
	.size	.L.str.8, 21

	.type	.L__chaos_bc_add,@object        # @__chaos_bc_add
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
.L__chaos_bc_add:
	.ascii	"CVM\000\003\000\002\000\f\000\000\000\022\000\000\000\240\000\240\001 \021\002\000\020\002\000\220"
	.size	.L__chaos_bc_add, 28

	.type	.Lstr,@object                   # @str
	.section	.rodata.str1.1,"aMS",@progbits,1
.Lstr:
	.asciz	"Testing add(3,7)..."
	.size	.Lstr, 20

	.type	.Lstr.9,@object                 # @str.9
.Lstr.9:
	.asciz	"Testing add(0,0)..."
	.size	.Lstr.9, 20

	.type	.Lstr.10,@object                # @str.10
.Lstr.10:
	.asciz	"Testing fib(10)..."
	.size	.Lstr.10, 19

	.type	.Lstr.11,@object                # @str.11
.Lstr.11:
	.asciz	"Testing fib(0)..."
	.size	.Lstr.11, 18

	.ident	"Ubuntu clang version 17.0.6 (9ubuntu1)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym .L__chaos_bc_add
