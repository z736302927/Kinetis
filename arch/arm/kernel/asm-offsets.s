	.arch armv7-m
	.eabi_attribute 20, 1	@ Tag_ABI_FP_denormal
	.eabi_attribute 21, 1	@ Tag_ABI_FP_exceptions
	.eabi_attribute 23, 3	@ Tag_ABI_FP_number_model
	.eabi_attribute 24, 1	@ Tag_ABI_align8_needed
	.eabi_attribute 25, 1	@ Tag_ABI_align8_preserved
	.eabi_attribute 26, 2	@ Tag_ABI_enum_size
	.eabi_attribute 30, 4	@ Tag_ABI_optimization_goals
	.eabi_attribute 34, 0	@ Tag_CPU_unaligned_access
	.eabi_attribute 18, 2	@ Tag_ABI_PCS_wchar_t
	.file	"asm-offsets.c"
@ GNU C89 (GNU Arm Embedded Toolchain 10-2020-q4-major) version 10.2.1 20201103 (release) (arm-none-eabi)
@	compiled by GNU C version 4.8.4, GMP version 6.1.0, MPFR version 3.1.4, MPC version 1.0.3, isl version isl-0.18-GMP

@ GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
@ options passed:  -nostdinc -I ./arch/arm/include
@ -I ./arch/arm/include/generated -I ./include -I ./arch/arm/include/uapi
@ -I ./arch/arm/include/generated/uapi -I ./include/uapi
@ -I ./include/generated/uapi -imultilib thumb/v7-m/nofp
@ -iprefix /home/yaqi/Software/gcc-arm-none-eabi-10-2020-q4-major/bin/../lib/gcc/arm-none-eabi/10.2.1/
@ -isysroot /home/yaqi/Software/gcc-arm-none-eabi-10-2020-q4-major/bin/../arm-none-eabi
@ -D__USES_INITFINI__ -D __KERNEL__ -D __LINUX_ARM_ARCH__=7 -U arm
@ -D KBUILD_MODFILE="./asm-offsets" -D KBUILD_BASENAME="asm_offsets"
@ -D KBUILD_MODNAME="asm_offsets"
@ -isystem /home/yaqi/Software/gcc-arm-none-eabi-10-2020-q4-major/bin/../lib/gcc/arm-none-eabi/10.2.1/include
@ -include ./include/linux/kconfig.h
@ -include ./include/linux/compiler_types.h
@ -MMD arch/arm/kernel/.asm-offsets.s.d arch/arm/kernel/asm-offsets.c
@ -mlittle-endian -mno-unaligned-access -mabi=aapcs-linux -mfpu=vfp -mthumb
@ -mfloat-abi=soft -march=armv7-m
@ -auxbase-strip arch/arm/kernel/asm-offsets.s -Os -Wall -Wundef
@ -Werror=strict-prototypes -Wno-trigraphs
@ -Werror=implicit-function-declaration -Werror=implicit-int
@ -Werror=return-type -Wno-format-security -Wno-frame-address
@ -Wformat-truncation=0 -Wformat-overflow=0 -Wno-address-of-packed-member
@ -Wframe-larger-than=1024 -Wimplicit-fallthrough=3
@ -Wno-unused-but-set-variable -Wunused-const-variable=0
@ -Wdeclaration-after-statement -Wvla -Wno-pointer-sign
@ -Wno-stringop-truncation -Wno-zero-length-bounds -Wno-array-bounds
@ -Wstringop-overflow=0 -Wno-restrict -Wno-maybe-uninitialized
@ -Werror=date-time -Werror=incompatible-pointer-types
@ -Werror=designated-init -Wno-packed-not-aligned -std=gnu90
@ -fmacro-prefix-map=./= -fno-strict-aliasing -fno-common -fshort-wchar
@ -fno-PIE -fno-dwarf2-cfi-asm -fno-ipa-sra -funwind-tables
@ -fno-delete-null-pointer-checks -fno-allow-store-data-races
@ -fstack-protector-strong -fomit-frame-pointer -fno-strict-overflow
@ -fstack-check=no -fconserve-stack -fverbose-asm
@ options enabled:  -faggressive-loop-optimizations -fallocation-dce
@ -fauto-inc-dec -fbranch-count-reg -fcaller-saves -fcode-hoisting
@ -fcombine-stack-adjustments -fcompare-elim -fcprop-registers
@ -fcrossjumping -fcse-follow-jumps -fdefer-pop -fdevirtualize
@ -fdevirtualize-speculatively -fearly-inlining
@ -feliminate-unused-debug-symbols -feliminate-unused-debug-types
@ -fexpensive-optimizations -fforward-propagate -ffp-int-builtin-inexact
@ -ffunction-cse -fgcse -fgcse-lm -fgnu-unique -fguess-branch-probability
@ -fhoist-adjacent-loads -fident -fif-conversion -fif-conversion2
@ -findirect-inlining -finline -finline-atomics -finline-functions
@ -finline-functions-called-once -finline-small-functions -fipa-bit-cp
@ -fipa-cp -fipa-icf -fipa-icf-functions -fipa-icf-variables -fipa-profile
@ -fipa-pure-const -fipa-ra -fipa-reference -fipa-reference-addressable
@ -fipa-stack-alignment -fipa-vrp -fira-hoist-pressure
@ -fira-share-save-slots -fira-share-spill-slots
@ -fisolate-erroneous-paths-dereference -fivopts -fkeep-static-consts
@ -fleading-underscore -flifetime-dse -flra-remat -fmath-errno
@ -fmerge-constants -fmerge-debug-strings -fmove-loop-invariants
@ -fomit-frame-pointer -foptimize-sibling-calls -fpartial-inlining
@ -fpeephole -fpeephole2 -fplt -fprefetch-loop-arrays -freg-struct-return
@ -freorder-blocks -freorder-functions -frerun-cse-after-loop
@ -fsched-critical-path-heuristic -fsched-dep-count-heuristic
@ -fsched-group-heuristic -fsched-interblock -fsched-last-insn-heuristic
@ -fsched-pressure -fsched-rank-heuristic -fsched-spec
@ -fsched-spec-insn-heuristic -fsched-stalled-insns-dep -fschedule-insns2
@ -fsection-anchors -fsemantic-interposition -fshow-column
@ -fshrink-wrap-separate -fsigned-zeros -fsplit-ivs-in-unroller
@ -fsplit-wide-types -fssa-backprop -fssa-phiopt -fstack-protector-strong
@ -fstdarg-opt -fstore-merging -fstrict-volatile-bitfields -fsync-libcalls
@ -fthread-jumps -ftoplevel-reorder -ftrapping-math -ftree-bit-ccp
@ -ftree-builtin-call-dce -ftree-ccp -ftree-ch -ftree-coalesce-vars
@ -ftree-copy-prop -ftree-cselim -ftree-dce -ftree-dominator-opts
@ -ftree-dse -ftree-forwprop -ftree-fre -ftree-loop-distribute-patterns
@ -ftree-loop-if-convert -ftree-loop-im -ftree-loop-ivcanon
@ -ftree-loop-optimize -ftree-parallelize-loops= -ftree-phiprop -ftree-pre
@ -ftree-pta -ftree-reassoc -ftree-scev-cprop -ftree-sink -ftree-slsr
@ -ftree-sra -ftree-switch-conversion -ftree-tail-merge -ftree-ter
@ -ftree-vrp -funit-at-a-time -funwind-tables -fverbose-asm -fwrapv
@ -fwrapv-pointer -fzero-initialized-in-bss -masm-syntax-unified -mbe32
@ -mlittle-endian -mpic-data-is-text-relative -msched-prolog -mthumb
@ -mvectorize-with-neon-quad

	.text
	.syntax unified
	.syntax unified
	.thumb
	.syntax unified
	.section	.text.startup,"ax",%progbits
	.align	1
	.global	main
	.syntax unified
	.thumb
	.thumb_func
	.fpu softvfp
	.type	main, %function
main:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
@ arch/arm/kernel/asm-offsets.c:39:   DEFINE(TSK_ACTIVE_MM,		offsetof(struct task_struct, active_mm));
	.syntax unified
@ 39 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->TSK_ACTIVE_MM #404 offsetof(struct task_struct, active_mm)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:41:   DEFINE(TSK_STACK_CANARY,	offsetof(struct task_struct, stack_canary));
@ 41 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->TSK_STACK_CANARY #520 offsetof(struct task_struct, stack_canary)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:43:   BLANK();
@ 43 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:44:   DEFINE(TI_FLAGS,		offsetof(struct thread_info, flags));
@ 44 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->TI_FLAGS #0 offsetof(struct thread_info, flags)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:45:   DEFINE(TI_PREEMPT,		offsetof(struct thread_info, preempt_count));
@ 45 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->TI_PREEMPT #4 offsetof(struct thread_info, preempt_count)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:46:   DEFINE(TI_ADDR_LIMIT,		offsetof(struct thread_info, addr_limit));
@ 46 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->TI_ADDR_LIMIT #8 offsetof(struct thread_info, addr_limit)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:47:   DEFINE(TI_TASK,		offsetof(struct thread_info, task));
@ 47 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->TI_TASK #12 offsetof(struct thread_info, task)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:48:   DEFINE(TI_CPU,		offsetof(struct thread_info, cpu));
@ 48 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->TI_CPU #16 offsetof(struct thread_info, cpu)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:49:   DEFINE(TI_CPU_DOMAIN,		offsetof(struct thread_info, cpu_domain));
@ 49 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->TI_CPU_DOMAIN #20 offsetof(struct thread_info, cpu_domain)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:50:   DEFINE(TI_CPU_SAVE,		offsetof(struct thread_info, cpu_context));
@ 50 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->TI_CPU_SAVE #24 offsetof(struct thread_info, cpu_context)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:51:   DEFINE(TI_USED_CP,		offsetof(struct thread_info, used_cp));
@ 51 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->TI_USED_CP #76 offsetof(struct thread_info, used_cp)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:52:   DEFINE(TI_TP_VALUE,		offsetof(struct thread_info, tp_value));
@ 52 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->TI_TP_VALUE #92 offsetof(struct thread_info, tp_value)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:53:   DEFINE(TI_FPSTATE,		offsetof(struct thread_info, fpstate));
@ 53 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->TI_FPSTATE #104 offsetof(struct thread_info, fpstate)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:72:   DEFINE(THREAD_SZ_ORDER,	THREAD_SIZE_ORDER);
@ 72 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->THREAD_SZ_ORDER #1 THREAD_SIZE_ORDER"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:73:   BLANK();
@ 73 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:74:   DEFINE(S_R0,			offsetof(struct pt_regs, ARM_r0));
@ 74 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_R0 #0 offsetof(struct pt_regs, ARM_r0)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:75:   DEFINE(S_R1,			offsetof(struct pt_regs, ARM_r1));
@ 75 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_R1 #4 offsetof(struct pt_regs, ARM_r1)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:76:   DEFINE(S_R2,			offsetof(struct pt_regs, ARM_r2));
@ 76 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_R2 #8 offsetof(struct pt_regs, ARM_r2)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:77:   DEFINE(S_R3,			offsetof(struct pt_regs, ARM_r3));
@ 77 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_R3 #12 offsetof(struct pt_regs, ARM_r3)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:78:   DEFINE(S_R4,			offsetof(struct pt_regs, ARM_r4));
@ 78 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_R4 #16 offsetof(struct pt_regs, ARM_r4)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:79:   DEFINE(S_R5,			offsetof(struct pt_regs, ARM_r5));
@ 79 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_R5 #20 offsetof(struct pt_regs, ARM_r5)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:80:   DEFINE(S_R6,			offsetof(struct pt_regs, ARM_r6));
@ 80 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_R6 #24 offsetof(struct pt_regs, ARM_r6)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:81:   DEFINE(S_R7,			offsetof(struct pt_regs, ARM_r7));
@ 81 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_R7 #28 offsetof(struct pt_regs, ARM_r7)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:82:   DEFINE(S_R8,			offsetof(struct pt_regs, ARM_r8));
@ 82 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_R8 #32 offsetof(struct pt_regs, ARM_r8)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:83:   DEFINE(S_R9,			offsetof(struct pt_regs, ARM_r9));
@ 83 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_R9 #36 offsetof(struct pt_regs, ARM_r9)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:84:   DEFINE(S_R10,			offsetof(struct pt_regs, ARM_r10));
@ 84 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_R10 #40 offsetof(struct pt_regs, ARM_r10)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:85:   DEFINE(S_FP,			offsetof(struct pt_regs, ARM_fp));
@ 85 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_FP #44 offsetof(struct pt_regs, ARM_fp)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:86:   DEFINE(S_IP,			offsetof(struct pt_regs, ARM_ip));
@ 86 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_IP #48 offsetof(struct pt_regs, ARM_ip)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:87:   DEFINE(S_SP,			offsetof(struct pt_regs, ARM_sp));
@ 87 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_SP #52 offsetof(struct pt_regs, ARM_sp)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:88:   DEFINE(S_LR,			offsetof(struct pt_regs, ARM_lr));
@ 88 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_LR #56 offsetof(struct pt_regs, ARM_lr)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:89:   DEFINE(S_PC,			offsetof(struct pt_regs, ARM_pc));
@ 89 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_PC #60 offsetof(struct pt_regs, ARM_pc)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:90:   DEFINE(S_PSR,			offsetof(struct pt_regs, ARM_cpsr));
@ 90 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_PSR #64 offsetof(struct pt_regs, ARM_cpsr)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:91:   DEFINE(S_OLD_R0,		offsetof(struct pt_regs, ARM_ORIG_r0));
@ 91 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->S_OLD_R0 #68 offsetof(struct pt_regs, ARM_ORIG_r0)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:92:   DEFINE(PT_REGS_SIZE,		sizeof(struct pt_regs));
@ 92 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->PT_REGS_SIZE #72 sizeof(struct pt_regs)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:93:   DEFINE(SVC_DACR,		offsetof(struct svc_pt_regs, dacr));
@ 93 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->SVC_DACR #72 offsetof(struct svc_pt_regs, dacr)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:94:   DEFINE(SVC_ADDR_LIMIT,	offsetof(struct svc_pt_regs, addr_limit));
@ 94 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->SVC_ADDR_LIMIT #76 offsetof(struct svc_pt_regs, addr_limit)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:95:   DEFINE(SVC_REGS_SIZE,		sizeof(struct svc_pt_regs));
@ 95 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->SVC_REGS_SIZE #80 sizeof(struct svc_pt_regs)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:96:   BLANK();
@ 96 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:97:   DEFINE(SIGFRAME_RC3_OFFSET,	offsetof(struct sigframe, retcode[3]));
@ 97 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->SIGFRAME_RC3_OFFSET #756 offsetof(struct sigframe, retcode[3])"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:98:   DEFINE(RT_SIGFRAME_RC3_OFFSET, offsetof(struct rt_sigframe, sig.retcode[3]));
@ 98 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->RT_SIGFRAME_RC3_OFFSET #884 offsetof(struct rt_sigframe, sig.retcode[3])"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:99:   BLANK();
@ 99 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:115:   DEFINE(VMA_VM_MM,		offsetof(struct vm_area_struct, vm_mm));
@ 115 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->VMA_VM_MM #32 offsetof(struct vm_area_struct, vm_mm)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:116:   DEFINE(VMA_VM_FLAGS,		offsetof(struct vm_area_struct, vm_flags));
@ 116 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->VMA_VM_FLAGS #40 offsetof(struct vm_area_struct, vm_flags)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:117:   BLANK();
@ 117 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:118:   DEFINE(VM_EXEC,	       	VM_EXEC);
@ 118 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->VM_EXEC #4 VM_EXEC"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:119:   BLANK();
@ 119 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:120:   DEFINE(PAGE_SZ,	       	PAGE_SIZE);
@ 120 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->PAGE_SZ #4096 PAGE_SIZE"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:121:   BLANK();
@ 121 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:122:   DEFINE(SYS_ERROR0,		0x9f0000);
@ 122 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->SYS_ERROR0 #10420224 0x9f0000"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:123:   BLANK();
@ 123 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:124:   DEFINE(SIZEOF_MACHINE_DESC,	sizeof(struct machine_desc));
@ 124 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->SIZEOF_MACHINE_DESC #100 sizeof(struct machine_desc)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:125:   DEFINE(MACHINFO_TYPE,		offsetof(struct machine_desc, nr));
@ 125 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->MACHINFO_TYPE #0 offsetof(struct machine_desc, nr)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:126:   DEFINE(MACHINFO_NAME,		offsetof(struct machine_desc, name));
@ 126 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->MACHINFO_NAME #4 offsetof(struct machine_desc, name)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:127:   BLANK();
@ 127 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:128:   DEFINE(PROC_INFO_SZ,		sizeof(struct proc_info_list));
@ 128 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->PROC_INFO_SZ #52 sizeof(struct proc_info_list)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:129:   DEFINE(PROCINFO_INITFUNC,	offsetof(struct proc_info_list, __cpu_flush));
@ 129 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->PROCINFO_INITFUNC #16 offsetof(struct proc_info_list, __cpu_flush)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:130:   DEFINE(PROCINFO_MM_MMUFLAGS,	offsetof(struct proc_info_list, __cpu_mm_mmu_flags));
@ 130 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->PROCINFO_MM_MMUFLAGS #8 offsetof(struct proc_info_list, __cpu_mm_mmu_flags)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:131:   DEFINE(PROCINFO_IO_MMUFLAGS,	offsetof(struct proc_info_list, __cpu_io_mmu_flags));
@ 131 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->PROCINFO_IO_MMUFLAGS #12 offsetof(struct proc_info_list, __cpu_io_mmu_flags)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:132:   BLANK();
@ 132 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:145:   DEFINE(CACHE_FLUSH_KERN_ALL,	offsetof(struct cpu_cache_fns, flush_kern_all));
@ 145 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->CACHE_FLUSH_KERN_ALL #4 offsetof(struct cpu_cache_fns, flush_kern_all)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:148:   DEFINE(SLEEP_SAVE_SP_SZ,	sizeof(struct sleep_save_sp));
@ 148 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->SLEEP_SAVE_SP_SZ #8 sizeof(struct sleep_save_sp)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:149:   DEFINE(SLEEP_SAVE_SP_PHYS,	offsetof(struct sleep_save_sp, save_ptr_stash_phys));
@ 149 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->SLEEP_SAVE_SP_PHYS #4 offsetof(struct sleep_save_sp, save_ptr_stash_phys)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:150:   DEFINE(SLEEP_SAVE_SP_VIRT,	offsetof(struct sleep_save_sp, save_ptr_stash));
@ 150 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->SLEEP_SAVE_SP_VIRT #0 offsetof(struct sleep_save_sp, save_ptr_stash)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:152:   DEFINE(ARM_SMCCC_QUIRK_ID_OFFS,	offsetof(struct arm_smccc_quirk, id));
@ 152 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->ARM_SMCCC_QUIRK_ID_OFFS #0 offsetof(struct arm_smccc_quirk, id)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:153:   DEFINE(ARM_SMCCC_QUIRK_STATE_OFFS,	offsetof(struct arm_smccc_quirk, state));
@ 153 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->ARM_SMCCC_QUIRK_STATE_OFFS #4 offsetof(struct arm_smccc_quirk, state)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:154:   BLANK();
@ 154 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:155:   DEFINE(DMA_BIDIRECTIONAL,	DMA_BIDIRECTIONAL);
@ 155 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->DMA_BIDIRECTIONAL #0 DMA_BIDIRECTIONAL"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:156:   DEFINE(DMA_TO_DEVICE,		DMA_TO_DEVICE);
@ 156 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->DMA_TO_DEVICE #1 DMA_TO_DEVICE"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:157:   DEFINE(DMA_FROM_DEVICE,	DMA_FROM_DEVICE);
@ 157 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->DMA_FROM_DEVICE #2 DMA_FROM_DEVICE"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:158:   BLANK();
@ 158 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:159:   DEFINE(CACHE_WRITEBACK_ORDER, __CACHE_WRITEBACK_ORDER);
@ 159 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->CACHE_WRITEBACK_ORDER #6 __CACHE_WRITEBACK_ORDER"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:160:   DEFINE(CACHE_WRITEBACK_GRANULE, __CACHE_WRITEBACK_GRANULE);
@ 160 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->CACHE_WRITEBACK_GRANULE #64 __CACHE_WRITEBACK_GRANULE"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:161:   BLANK();
@ 161 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:165:   BLANK();
@ 165 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->"
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:177:   DEFINE(KEXEC_START_ADDR,	offsetof(struct kexec_relocate_data, kexec_start_address));
@ 177 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->KEXEC_START_ADDR #0 offsetof(struct kexec_relocate_data, kexec_start_address)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:178:   DEFINE(KEXEC_INDIR_PAGE,	offsetof(struct kexec_relocate_data, kexec_indirection_page));
@ 178 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->KEXEC_INDIR_PAGE #4 offsetof(struct kexec_relocate_data, kexec_indirection_page)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:179:   DEFINE(KEXEC_MACH_TYPE,	offsetof(struct kexec_relocate_data, kexec_mach_type));
@ 179 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->KEXEC_MACH_TYPE #8 offsetof(struct kexec_relocate_data, kexec_mach_type)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:180:   DEFINE(KEXEC_R2,		offsetof(struct kexec_relocate_data, kexec_r2));
@ 180 "arch/arm/kernel/asm-offsets.c" 1
	
.ascii "->KEXEC_R2 #12 offsetof(struct kexec_relocate_data, kexec_r2)"	@
@ 0 "" 2
@ arch/arm/kernel/asm-offsets.c:182: }
	.thumb
	.syntax unified
	movs	r0, #0	@,
	bx	lr	@
	.fnend
	.size	main, .-main
	.ident	"GCC: (GNU Arm Embedded Toolchain 10-2020-q4-major) 10.2.1 20201103 (release)"
