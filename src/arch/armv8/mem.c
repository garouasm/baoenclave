/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#include <mem.h>
#include <cpu.h>
#include <arch/sysregs.h>

#include <fences.h>

void as_arch_init(addr_space_t* as)
{
    uint64_t index;

    /*
     * If the address space is a copy of an existing hypervisor space it's not
     * possible to use the PT_CPU_REC index to navigate it, so we have to use
     * the PT_VM_REC_IND.
     */
    if (as->type == AS_HYP_CPY || as->type == AS_VM) {
        index = (BAO_VAS_BASE >> 39) - as->id;
    } else {
        index = PT_CPU_REC_IND;
    }
    pt_set_recursive(&as->pt, index);
}

bool mem_translate(addr_space_t* as, void* va, uint64_t* pa)
{
    uint64_t par = 0, par_saved = 0;

    /**
     * TODO: are barriers needed in this operation?
     */

    par_saved = MRS(PAR_EL1);  // Move to ARM register from system coprocessor
                               // register. You can use this pseudo-instruction
                               // to read CP14 or CP15 coprocessor registers,
                               // with the exception of write-only registers

    if (as->type == AS_HYP || as->type == AS_HYP_CPY)
        asm volatile("AT S1E2W, %0" ::"r"(
            va));  // If EL2 is not implemented, the AT S1E2R and AT S1E2W
                   // instructions are UNDEFINED.
    else
        asm volatile("AT S12E1W, %0" ::"r"(
            va));  // Performs stage 1 and 2 address translation, with
                   // permissions as if writing to the given virtual address
                   // from EL1, or from EL2 if the Effective value of HCR_EL2.

    par = MRS(PAR_EL1);
    MSR(PAR_EL1, par_saved);
    if (par & PAR_F) {
        return false;
    } else {
        if (pa != NULL)
            *pa = (par & PAR_PA_MSK) | (((uint64_t)va) & (PAGE_SIZE - 1));
        return true;
    }
}

void mem_guest_ipa_translate(void* va, uint64_t* physical_address)
{
    uint64_t tmp = 0, tmp2 = 0;
    tmp = MRS(SCTLR_EL1);
    tmp2 = tmp & ~(1ULL << 0);
    MSR(SCTLR_EL1, tmp2);
    ISB();
    asm volatile("AT S12E1W, %0" ::"r"(va));
    ISB();
    MSR(SCTLR_EL1, tmp);
    *physical_address =
        (MRS(PAR_EL1) & PAR_PA_MSK) | (((uint64_t)va) & (PAGE_SIZE - 1));
}
