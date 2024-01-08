#include "xpf.h"

uint64_t xpf_find_arm_vm_init(void)
{
	PFStringMetric *contiguousHintMetric = pfmetric_string_init("use_contiguous_hint");
	__block uint64_t contiguousHintAddr = 0;
	pfmetric_run(gXPF.kernelStringSection, contiguousHintMetric, ^(uint64_t vmaddr, bool *stop) {
		contiguousHintAddr = vmaddr;
		*stop = true;
	});
	pfmetric_free(contiguousHintMetric);

	__block uint64_t arm_init_mid = 0;
	PFXrefMetric *contiguousHintXrefMetric = pfmetric_xref_init(contiguousHintAddr, XREF_TYPE_MASK_REFERENCE);
	pfmetric_run(gXPF.kernelTextSection, contiguousHintXrefMetric, ^(uint64_t vmaddr, bool *stop) {
		arm_init_mid = vmaddr;
		*stop = true;
	});
	pfmetric_free(contiguousHintXrefMetric);

	return pfsec_find_function_start(gXPF.kernelTextSection, arm_init_mid);
}

uint64_t xpf_find_arm_vm_init_reference(uint32_t n)
{
	uint64_t arm_vm_init = xpf_item_resolve("kernelSymbol.arm_vm_init");

	uint32_t strAny = 0, strAnyMask = 0;
	arm64_gen_str_imm(0, LDR_STR_TYPE_UNSIGNED, ARM64_REG_ANY, ARM64_REG_ANY, OPT_UINT64_NONE, &strAny, &strAnyMask);

	uint64_t toCheck = arm_vm_init;
	uint64_t strAddr = 0;
	for (int i = 0; i < n; i++) {
		strAddr = pfsec_find_next_inst(gXPF.kernelTextSection, toCheck, 0x20, strAny, strAnyMask);
		toCheck = strAddr + 4;
	}

	return pfsec_arm64_resolve_adrp_ldr_str_add_reference_auto(gXPF.kernelTextSection, strAddr);
}

uint64_t xpf_find_pmap_bootstrap(void)
{
	__block uint64_t pmap_asid_plru_stringAddr = 0;
	PFStringMetric *asidPlruMetric = pfmetric_string_init("pmap_asid_plru");
	pfmetric_run(gXPF.kernelStringSection, asidPlruMetric, ^(uint64_t vmaddr, bool *stop) {
		pmap_asid_plru_stringAddr = vmaddr;
		*stop = true;
	});
	pfmetric_free(asidPlruMetric);

	__block uint64_t pmap_bootstrap = 0;
	PFXrefMetric *asidPlruXrefMetric = pfmetric_xref_init(pmap_asid_plru_stringAddr, XREF_TYPE_MASK_REFERENCE);
	pfmetric_run(gXPF.kernelTextSection, asidPlruXrefMetric, ^(uint64_t vmaddr, bool *stop) {
		pmap_bootstrap = pfsec_find_function_start(gXPF.kernelTextSection, vmaddr);
		*stop = true;
	});
	pfmetric_free(asidPlruXrefMetric);
	return pmap_bootstrap;
}

uint64_t xpf_find_pointer_mask_symbol(uint32_t n)
{
	uint64_t pmap_bootstrap = xpf_item_resolve("kernelSymbol.pmap_bootstrap");

	uint32_t ldrQ0AnyInst = 0, ldrQ0AnyMask = 0;
	arm64_gen_ldr_imm(0, LDR_STR_TYPE_UNSIGNED, ARM64_REG_Q(0), ARM64_REG_ANY, OPT_UINT64_NONE, &ldrQ0AnyInst, &ldrQ0AnyMask);

	uint64_t ldrAddr = pfsec_find_next_inst(gXPF.kernelTextSection, pmap_bootstrap, 0x100, ldrQ0AnyInst, ldrQ0AnyMask);

	return pfsec_arm64_resolve_adrp_ldr_str_add_reference_auto(gXPF.kernelTextSection, ldrAddr);
}

uint64_t xpf_find_pointer_mask(void)
{
	uint64_t pointer_mask = pfsec_read64(gXPF.kernelConstSection, xpf_item_resolve("kernelSymbol.pointer_mask"));
	if (pointer_mask != 0xffffff8000000000 && pointer_mask != 0xffff800000000000 && pointer_mask != 0xffffffc000000000) {
		xpf_set_error("xpf_find_pointer_mask error: Unexpected PAC mask: 0x%llx", pointer_mask);
		return 0;
	}
	return pointer_mask;
}

uint64_t xpf_find_T1SZ_BOOT(void)
{
	// for T1SZ_BOOT, count how many bits in the pointer_mask are set
	uint64_t pointer_mask = xpf_item_resolve("kernelConstant.pointer_mask");
	uint64_t T1SZ_BOOT = 0;
	for (uint64_t i = 64; i > 0; i--) {
		if (pointer_mask & (1ULL << (i - 1))) {
			T1SZ_BOOT++;
		}
	}
	return T1SZ_BOOT;
}

uint64_t xpf_find_ARM_16K_TT_L1_INDEX_MASK(void)
{
	uint64_t T1SZ_BOOT = xpf_item_resolve("kernelConstant.T1SZ_BOOT");
	switch (T1SZ_BOOT) {
		case 17:
		return 0x00007ff000000000ULL;
		case 25:
		return 0x0000007000000000ULL;
		case 26:
		return 0x0000003fc0000000ULL;
		default:
		printf("ARM_16K_TT_L1_INDEX_MASK: Unexpected T1SZ_BOOT??? (%llu)\n", T1SZ_BOOT);
	}
	return 0;
}

uint64_t xpf_find_phystokv(void)
{
	uint64_t arm_vm_init = xpf_item_resolve("kernelSymbol.arm_vm_init");

	uint32_t blAny = 0, blAnyMask = 0;
	arm64_gen_b_l(OPT_BOOL(true), OPT_UINT64_NONE, OPT_UINT64_NONE, &blAny, &blAnyMask);

	uint64_t blPhystokvAddr = pfsec_find_next_inst(gXPF.kernelTextSection, arm_vm_init, 100, blAny, blAnyMask);
	uint64_t phystokv = 0;
	arm64_dec_b_l(pfsec_read32(gXPF.kernelTextSection, blPhystokvAddr), blPhystokvAddr, &phystokv, NULL);
	return phystokv;
}

/*uint64_t xpf_find_gVirtSize(void)
{
	return 0;
}*/

uint64_t xpf_find_ptov_table(void)
{
	uint64_t phystokv = xpf_item_resolve("kernelSymbol.phystokv");

	uint32_t ldrAny = 0, ldrAnyMask = 0;
	arm64_gen_ldr_imm(0, LDR_STR_TYPE_UNSIGNED, ARM64_REG_ANY, ARM64_REG_ANY, OPT_UINT64_NONE, &ldrAny, &ldrAnyMask);

	// Second ldr in phytokv references ptov_table
	uint64_t toCheck = phystokv;
	uint64_t ldrAddr = 0;
	for (int i = 0; i < 2; i++) {
		ldrAddr = pfsec_find_next_inst(gXPF.kernelTextSection, toCheck, 20, ldrAny, ldrAnyMask);
		toCheck = ldrAddr + 4;
	}

	return pfsec_arm64_resolve_adrp_ldr_str_add_reference_auto(gXPF.kernelTextSection, ldrAddr);
}

uint64_t xpf_find_start_first_cpu(void)
{
	uint64_t start_first_cpu = 0;
	arm64_dec_b_l(pfsec_read32(gXPF.kernelTextSection, gXPF.kernelEntry), gXPF.kernelEntry, &start_first_cpu, NULL);
	return start_first_cpu;
}

uint64_t xpf_find_cpu_ttep(void)
{
	uint64_t start_first_cpu = xpf_item_resolve("kernelSymbol.start_first_cpu");

	uint32_t cbzX21Any = 0, cbzX21AnyMask = 0;
	arm64_gen_cb_n_z(OPT_BOOL(false), ARM64_REG_X(21), OPT_UINT64_NONE, &cbzX21Any, &cbzX21AnyMask);

	uint64_t cpu_ttep_pre = pfsec_find_next_inst(gXPF.kernelTextSection, start_first_cpu, 0, cbzX21Any, cbzX21AnyMask);
	uint64_t addAddr = cpu_ttep_pre + 8;
	return pfsec_arm64_resolve_adrp_ldr_str_add_reference_auto(gXPF.kernelTextSection, addAddr);
}

uint64_t xpf_find_kernel_el(void)
{
	uint64_t start_first_cpu = xpf_item_resolve("kernelSymbol.start_first_cpu");

	uint32_t inst = pfsec_read32(gXPF.kernelTextSection, start_first_cpu + 16);
	if (inst == 0xd5384240 /* msr x0, CurrentEL */) {
		return 2;
	}

	return 1;
}

uint64_t xpf_find_fatal_error_fmt(void)
{
	PFSection *textSec = (gXPF.kernelAMFITextSection ?: gXPF.kernelTextSection);
	PFSection *stringSec = (gXPF.kernelAMFIStringSection ?: gXPF.kernelStringSection);
	if (!gXPF.kernelIsArm64e && !gXPF.kernelIsFileset) {
		textSec = gXPF.kernelPLKTextSection;
		stringSec = gXPF.kernelPrelinkTextSection;
	}

	PFStringMetric *amfiErrorMetric = pfmetric_string_init("AMFI: %s: Failed to allocate memory for fatal error message, cannot produce a crash reason.\n");
	__block uint64_t amfiErrorAddr = 0;
	pfmetric_run(stringSec, amfiErrorMetric, ^(uint64_t vmaddr, bool *stop) {
		amfiErrorAddr = vmaddr;
		*stop = true;
	});
	pfmetric_free(amfiErrorMetric);

	__block uint64_t fatal_error_fmt = 0;
	PFXrefMetric *amfiErrorXrefMetric = pfmetric_xref_init(amfiErrorAddr, XREF_TYPE_MASK_REFERENCE);
	pfmetric_run(textSec, amfiErrorXrefMetric, ^(uint64_t vmaddr, bool *stop) {
		fatal_error_fmt = pfsec_find_function_start(textSec, vmaddr);
		*stop = true;
	});
	pfmetric_free(amfiErrorXrefMetric);
	return fatal_error_fmt;
}

uint64_t xpf_find_kalloc_data_external(void)
{
	PFSection *sec = (gXPF.kernelAMFITextSection ?: gXPF.kernelTextSection);
	if (!gXPF.kernelIsArm64e && !gXPF.kernelIsFileset) {
		sec = gXPF.kernelPLKTextSection;
	}

	uint64_t fatal_error_fmt = xpf_item_resolve("kernelSymbol.fatal_error_fmt");

	uint32_t blAny = 0, blAnyMask = 0;
	arm64_gen_b_l(OPT_BOOL(true), OPT_UINT64_NONE, OPT_UINT64_NONE, &blAny, &blAnyMask);
	uint64_t kallocDataExternalBlAddr = pfsec_find_next_inst(sec, fatal_error_fmt, 20, blAny, blAnyMask);
	uint32_t kallocDataExternalBl = pfsec_read32(sec, kallocDataExternalBlAddr);

	uint64_t kallocDataExternal = 0;
	arm64_dec_b_l(kallocDataExternalBl, kallocDataExternalBlAddr, &kallocDataExternal, NULL);
	return pfsec_arm64_resolve_stub(sec, kallocDataExternal);
}

uint64_t xpf_find_kfree_data_external(void)
{
	PFSection *sec = (gXPF.kernelAMFITextSection ?: gXPF.kernelTextSection);
	if (!gXPF.kernelIsArm64e && !gXPF.kernelIsFileset) {
		sec = gXPF.kernelPLKTextSection;
	}

	uint64_t fatal_error_fmt = xpf_item_resolve("kernelSymbol.fatal_error_fmt");

	uint32_t blAny = 0, blAnyMask = 0;
	arm64_gen_b_l(OPT_BOOL(true), OPT_UINT64_NONE, OPT_UINT64_NONE, &blAny, &blAnyMask);

	uint32_t ret = gXPF.kernelIsArm64e ? 0xd65f0fff : 0xd65f03c0;
	uint64_t fatal_error_fmt_end = pfsec_find_next_inst(sec, fatal_error_fmt, 0, ret, 0xffffffff);
	if (fatal_error_fmt_end) {
		uint32_t movW1_0x400_Inst = 0, movW1_0x400_InstMask = 0;
		arm64_gen_mov_imm('z', ARM64_REG_W(1), OPT_UINT64(0x400), OPT_UINT64(0), &movW1_0x400_Inst, &movW1_0x400_InstMask);
		uint64_t kfree_data_external_pre = pfsec_find_prev_inst(sec, fatal_error_fmt_end, 25, movW1_0x400_Inst, movW1_0x400_InstMask);

		uint64_t kfree_data_external = 0;
		if (arm64_dec_b_l(pfsec_read32(sec, kfree_data_external_pre+4), kfree_data_external_pre+4, &kfree_data_external, NULL) == 0) {
			return pfsec_arm64_resolve_stub(sec, kfree_data_external);
		}
	}

	return 0;
}

uint64_t xpf_find_allproc(void)
{
	PFStringMetric *shutdownwaitMetric = pfmetric_string_init("shutdownwait");
	__block uint64_t shutdownwaitString = 0;
	pfmetric_run(gXPF.kernelStringSection, shutdownwaitMetric, ^(uint64_t vmaddr, bool *stop) {
		shutdownwaitString = vmaddr;
		*stop = true;
	});
	pfmetric_free(shutdownwaitMetric);

	PFXrefMetric *shutdownwaitXrefMetric = pfmetric_xref_init(shutdownwaitString, XREF_TYPE_MASK_REFERENCE);
	__block uint64_t shutdownwaitXref = 0;
	pfmetric_run(gXPF.kernelTextSection, shutdownwaitXrefMetric, ^(uint64_t vmaddr, bool *stop) {
		shutdownwaitXref = vmaddr;
		*stop = true;
	});
	pfmetric_free(shutdownwaitXrefMetric);

	uint32_t ldrAny = 0, ldrAnyMask = 0;
	arm64_gen_ldr_imm(0, LDR_STR_TYPE_UNSIGNED, ARM64_REG_ANY, ARM64_REG_ANY, OPT_UINT64_NONE, &ldrAny, &ldrAnyMask);

	uint64_t ldrAddr = pfsec_find_next_inst(gXPF.kernelTextSection, shutdownwaitXref, 20, ldrAny, ldrAnyMask);
	return pfsec_arm64_resolve_adrp_ldr_str_add_reference_auto(gXPF.kernelTextSection, ldrAddr);
}

uint64_t xpf_find_task_crashinfo_release_ref(void)
{
	uint32_t movzW0_0 = 0, movzW0_0Mask = 0;
	arm64_gen_mov_imm('z', ARM64_REG_W(0), OPT_UINT64(0), OPT_UINT64(0), &movzW0_0, &movzW0_0Mask);

	PFStringMetric *corpseReleasedMetric = pfmetric_string_init("Corpse released, count at %d\n");
	__block uint64_t corpseReleasedStringAddr = 0;
	pfmetric_run(gXPF.kernelOSLogSection, corpseReleasedMetric, ^(uint64_t vmaddr, bool *stop){
		corpseReleasedStringAddr = vmaddr;
		*stop = true;
	});
	pfmetric_free(corpseReleasedMetric);

	PFXrefMetric *corseReleasedXrefMetric = pfmetric_xref_init(corpseReleasedStringAddr, XREF_TYPE_MASK_REFERENCE);
	__block uint64_t task_crashinfo_release_ref = 0;
	pfmetric_run(gXPF.kernelTextSection, corseReleasedXrefMetric, ^(uint64_t vmaddr, bool *stop) {
		if (pfsec_find_next_inst(gXPF.kernelTextSection, vmaddr, 20, movzW0_0, movzW0_0Mask)) {
			task_crashinfo_release_ref = pfsec_find_function_start(gXPF.kernelTextSection, vmaddr);
			*stop = true;
		}
	});
	pfmetric_free(corseReleasedXrefMetric);

	return task_crashinfo_release_ref;
}

uint64_t xpf_find_task_collect_crash_info(void)
{
	uint64_t task_crashinfo_release_ref = xpf_item_resolve("kernelSymbol.task_crashinfo_release_ref");

	uint32_t movzW1_0x4000 = 0x52880001, movzW1_0x4000Mask = 0xffffffff;
	__block uint64_t task_collect_crash_info = 0;
	PFXrefMetric *task_crashinfo_release_refXrefMetric = pfmetric_xref_init(task_crashinfo_release_ref, XREF_TYPE_MASK_CALL);
	pfmetric_run(gXPF.kernelTextSection, task_crashinfo_release_refXrefMetric, ^(uint64_t vmaddr, bool *stop) {
		if (pfsec_find_prev_inst(gXPF.kernelTextSection, vmaddr, 0x50, movzW1_0x4000, movzW1_0x4000Mask)) {
			task_collect_crash_info = pfsec_find_function_start(gXPF.kernelTextSection, vmaddr);
			*stop = true;
		}
	});
	pfmetric_free(task_crashinfo_release_refXrefMetric);

	return task_collect_crash_info;
}

uint64_t xpf_find_task_itk_space(void)
{
	uint64_t task_collect_crash_info = xpf_item_resolve("kernelSymbol.task_collect_crash_info");

	uint32_t movzW2_1 = 0, movzW2_1Mask = 0;
	arm64_gen_mov_imm('z', ARM64_REG_W(2), OPT_UINT64(1), OPT_UINT64(0), &movzW2_1, &movzW2_1Mask);

	__block uint64_t itk_space = 0;
	PFXrefMetric *task_collect_crash_infoXrefMetric = pfmetric_xref_init(task_collect_crash_info, XREF_TYPE_MASK_CALL);
	pfmetric_run(gXPF.kernelTextSection, task_collect_crash_infoXrefMetric, ^(uint64_t vmaddr, bool *stop) {
		if ((pfsec_read32(gXPF.kernelTextSection, vmaddr - 4) & movzW2_1Mask) != movzW2_1) return;

		// At vmaddr + 4 there is a CBZ to some other place
		// At that place, the next CBZ leads to the place where the actual reference we want is

		uint64_t cbz1Addr = vmaddr + 4;
		bool isCbnz = false;
		uint64_t target1 = 0;
		if (arm64_dec_cb_n_z(pfsec_read32(gXPF.kernelTextSection, cbz1Addr), cbz1Addr, &isCbnz, NULL, &target1) != 0) {
			xpf_set_error("itk_space error: first branch is not cbz");
			*stop = true;
		}
		if (isCbnz) {
			// If this is not a cbz and rather a cbnz, treat the instruction after it as the cbz target
			target1 = cbz1Addr + 4;
		}

		uint32_t cbzAnyInst = 0, cbzAnyMask = 0;
		arm64_gen_cb_n_z(OPT_BOOL_NONE, ARM64_REG_ANY, OPT_UINT64_NONE, &cbzAnyInst, &cbzAnyMask);

		uint64_t cbz2Addr = pfsec_find_next_inst(gXPF.kernelTextSection, target1, 0x20, cbzAnyInst, cbzAnyMask);

		uint64_t target2 = 0;
		if (arm64_dec_cb_n_z(pfsec_read32(gXPF.kernelTextSection, cbz2Addr), cbz2Addr, &isCbnz, NULL, &target2) != 0) {
			xpf_set_error("itk_space error: second branch not found");
			*stop = true;
		}
		if (isCbnz) {
			// If this is not a cbz and rather a cbnz, treat the instruction after it as the cbz target
			target2 = cbz2Addr + 4;
		}

		uint32_t ldrAnyInst = 0, ldrAnyMask = 0;
		arm64_gen_ldr_imm(0, LDR_STR_TYPE_UNSIGNED, ARM64_REG_ANY, ARM64_REG_ANY, OPT_UINT64_NONE, &ldrAnyInst, &ldrAnyMask);

		// At this place, the first ldr that doesn't read from SP has the reference we want
		uint64_t ldrAddr = target2;
		while (true) {
			ldrAddr = pfsec_find_next_inst(gXPF.kernelTextSection, ldrAddr+4, 0, ldrAnyInst, ldrAnyMask);
			arm64_register addrReg;
			uint64_t imm = 0;
			arm64_dec_ldr_imm(pfsec_read32(gXPF.kernelTextSection, ldrAddr), NULL, &addrReg, &imm, NULL, NULL);
			if (ARM64_REG_GET_NUM(addrReg) != ARM64_REG_NUM_SP) {
				itk_space = imm;
				*stop = true;
				break;
			}
		}
	});
	pfmetric_free(task_collect_crash_infoXrefMetric);

	return itk_space;
}

uint64_t xpf_find_vm_reference(uint32_t idx)
{
	uint32_t inst = 0x120a6d28; /*and w8, w9, #0xffc3ffff*/
	PFPatternMetric *patternMetric = pfmetric_pattern_init(&inst, NULL, sizeof(inst), sizeof(uint32_t));
	__block uint64_t ref = 0;
	pfmetric_run(gXPF.kernelTextSection, patternMetric, ^(uint64_t vmaddr, bool *stop) {
		uint32_t ldrAny = 0, ldrAnyMask = 0;
		arm64_gen_ldr_imm(0, LDR_STR_TYPE_UNSIGNED, ARM64_REG_ANY, ARM64_REG_ANY, OPT_UINT64_NONE, &ldrAny, &ldrAnyMask);
		uint64_t toCheck = vmaddr;
		uint64_t ldrAddr = 0;
		for (int i = 0; i < idx; i++) {
			ldrAddr = pfsec_find_next_inst(gXPF.kernelTextSection, toCheck, 20, ldrAny, ldrAnyMask);
			toCheck = ldrAddr + 4;
		}

		ref = pfsec_arm64_resolve_adrp_ldr_str_add_reference_auto(gXPF.kernelTextSection, ldrAddr);
	});
	pfmetric_free(patternMetric);
	return ref;
}

uint64_t xpf_find_vm_map_pmap(void)
{
	PFStringMetric *stringMetric = pfmetric_string_init("userspace has control access to a kernel map %p through task %p @%s:%d");
	__block uint64_t stringAddr = 0;
	pfmetric_run(gXPF.kernelStringSection, stringMetric, ^(uint64_t vmaddr, bool *stop) {
		stringAddr = vmaddr;
		*stop = true;
	});
	pfmetric_free(stringMetric);

	PFXrefMetric *xrefMetric = pfmetric_xref_init(stringAddr, XREF_TYPE_MASK_REFERENCE);
	__block uint64_t xrefAddr = 0;
	pfmetric_run(gXPF.kernelTextSection, xrefMetric, ^(uint64_t vmaddr, bool *stop) {
		xrefAddr = vmaddr;
		*stop = true;
	});
	pfmetric_free(xrefMetric);

	uint32_t inst[2] = { 0 };
	uint32_t mask[2] = { 0 };
	arm64_gen_ldr_imm(0, gXPF.kernelIsArm64e ? LDR_STR_TYPE_PRE_INDEX : LDR_STR_TYPE_UNSIGNED, ARM64_REG_ANY, ARM64_REG_ANY, OPT_UINT64_NONE, &inst[0], &mask[0]);
	if (gXPF.kernelIsArm64e) {
		arm64_gen_cb_n_z(OPT_BOOL_NONE, ARM64_REG_ANY, OPT_UINT64_NONE, &inst[1], &mask[1]);
	}

	__block uint64_t vm_map_pmap = 0;
	PFPatternMetric *patternMetric = pfmetric_pattern_init(inst, mask, sizeof(inst), sizeof(uint32_t));
	pfmetric_run_in_range(gXPF.kernelTextSection, xrefAddr, xrefAddr - (100 * sizeof(uint32_t)), patternMetric, ^(uint64_t vmaddr, bool *stop) {
		if (arm64_dec_adr_p(pfsec_read32(gXPF.kernelTextSection, vmaddr - 4), vmaddr - 4, NULL, NULL, NULL) != 0) {
			arm64_dec_ldr_imm(pfsec_read32(gXPF.kernelTextSection, vmaddr), NULL, NULL, &vm_map_pmap, NULL, NULL);
			*stop = true;
		}
	});
	pfmetric_free(patternMetric);

	return vm_map_pmap;
}

void xpf_common_init(void)
{
	xpf_item_register("kernelSymbol.start_first_cpu", xpf_find_start_first_cpu, NULL);
	xpf_item_register("kernelConstant.kernel_el", xpf_find_kernel_el, NULL);
	xpf_item_register("kernelSymbol.cpu_ttep", xpf_find_cpu_ttep, NULL);
	xpf_item_register("kernelSymbol.fatal_error_fmt", xpf_find_fatal_error_fmt, NULL);
	xpf_item_register("kernelSymbol.kalloc_data_external", xpf_find_kalloc_data_external, NULL);
	xpf_item_register("kernelSymbol.kfree_data_external", xpf_find_kfree_data_external, NULL);
	xpf_item_register("kernelSymbol.allproc", xpf_find_allproc, NULL);

	xpf_item_register("kernelSymbol.arm_vm_init", xpf_find_arm_vm_init, NULL);
	xpf_item_register("kernelSymbol.phystokv", xpf_find_phystokv, NULL);

	xpf_item_register("kernelSymbol.gVirtBase", xpf_find_arm_vm_init_reference, (void*)(uint32_t)1);
	xpf_item_register("kernelSymbol.gPhysBase", xpf_find_arm_vm_init_reference, (void*)(uint32_t)2);
	xpf_item_register("kernelSymbol.gPhysSize", xpf_find_arm_vm_init_reference, (void*)(uint32_t)5);
	xpf_item_register("kernelSymbol.ptov_table", xpf_find_ptov_table, NULL);

	xpf_item_register("kernelSymbol.pmap_bootstrap", xpf_find_pmap_bootstrap, NULL);
	xpf_item_register("kernelSymbol.pointer_mask", xpf_find_pointer_mask_symbol, NULL);
	xpf_item_register("kernelConstant.pointer_mask", xpf_find_pointer_mask, NULL);
	xpf_item_register("kernelConstant.T1SZ_BOOT", xpf_find_T1SZ_BOOT, NULL);
	xpf_item_register("kernelConstant.ARM_16K_TT_L1_INDEX_MASK", xpf_find_ARM_16K_TT_L1_INDEX_MASK, NULL);

	xpf_item_register("kernelSymbol.vm_page_array_beginning_addr", xpf_find_vm_reference, (void *)(uint32_t)1);
	xpf_item_register("kernelSymbol.vm_page_array_ending_addr", xpf_find_vm_reference, (void *)(uint32_t)2);
	xpf_item_register("kernelSymbol.vm_first_phys_ppnum", xpf_find_vm_reference, (void *)(uint32_t)3);

	xpf_item_register("kernelSymbol.task_crashinfo_release_ref", xpf_find_task_crashinfo_release_ref, NULL);
	xpf_item_register("kernelSymbol.task_collect_crash_info", xpf_find_task_collect_crash_info, NULL);
	xpf_item_register("kernelStruct.task.itk_space", xpf_find_task_itk_space, NULL);

	xpf_item_register("kernelStruct.vm_map.pmap", xpf_find_vm_map_pmap, NULL);
}
