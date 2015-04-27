	.def	 @feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
@feat.00 = 1
	.def	 _fn_u0;
	.scl	2;
	.type	32;
	.endef
	.text
	.globl	_fn_u0
	.align	16, 0x90
_fn_u0:                                 # @fn_u0
	.cfi_startproc
# BB#0:
	calll	_fn_u1
	ret
	.cfi_endproc

	.def	 _fn_u1;
	.scl	2;
	.type	32;
	.endef
	.globl	_fn_u1
	.align	16, 0x90
_fn_u1:                                 # @fn_u1
	.cfi_startproc
# BB#0:
	pushl	%esi
Ltmp2:
	.cfi_def_cfa_offset 8
	subl	$16, %esp
Ltmp3:
	.cfi_def_cfa_offset 24
Ltmp4:
	.cfi_offset %esi, -8
	movl	$7, 4(%esp)
	movl	$21, (%esp)
	calll	_fn_u2
	movl	%eax, %esi
	movl	%esi, 12(%esp)
	movl	$9, 4(%esp)
	movl	$15, (%esp)
	calll	_fn_u2
	movl	%eax, 8(%esp)
	movl	%eax, 4(%esp)
	movl	%esi, (%esp)
	calll	_juStd_addInt
	movl	%eax, 12(%esp)
	movl	%eax, (%esp)
	calll	_fn_u3
	addl	$16, %esp
	popl	%esi
	ret
	.cfi_endproc

	.def	 _fn_u2;
	.scl	2;
	.type	32;
	.endef
	.globl	_fn_u2
	.align	16, 0x90
_fn_u2:                                 # @fn_u2
	.cfi_startproc
# BB#0:
	pushl	%esi
Ltmp7:
	.cfi_def_cfa_offset 8
	subl	$12, %esp
Ltmp8:
	.cfi_def_cfa_offset 20
Ltmp9:
	.cfi_offset %esi, -8
	movl	20(%esp), %esi
	movl	24(%esp), %eax
	movl	%eax, (%esp)
	calll	_juStd_negInt
	movl	%eax, 8(%esp)
	movl	%eax, 4(%esp)
	movl	%esi, (%esp)
	calll	_juStd_addInt
	addl	$12, %esp
	popl	%esi
	ret
	.cfi_endproc

	.def	 _fn_u3;
	.scl	2;
	.type	32;
	.endef
	.globl	_fn_u3
	.align	16, 0x90
_fn_u3:                                 # @fn_u3
	.cfi_startproc
# BB#0:
	pushl	%eax
Ltmp11:
	.cfi_def_cfa_offset 8
	movl	8(%esp), %eax
	movl	%eax, (%esp)
	calll	_juStd_printInt
	calll	_juStd_println
	popl	%edx
	ret
	.cfi_endproc

	.def	 _main;
	.scl	2;
	.type	32;
	.endef
	.globl	_main
	.align	16, 0x90
_main:                                  # @main
	.cfi_startproc
# BB#0:
	pushl	%ebp
Ltmp14:
	.cfi_def_cfa_offset 8
Ltmp15:
	.cfi_offset %ebp, -8
	movl	%esp, %ebp
Ltmp16:
	.cfi_def_cfa_register %ebp
	calll	___main
	calll	_fn_u0
	xorl	%eax, %eax
	popl	%ebp
	ret
	.cfi_endproc


