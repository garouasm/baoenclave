/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      David Cerdeira <davidmcerdeira@gmail.com>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#include <ipc.h>

#include <cpu.h>
#include <vmm.h>
#include <hypercall.h>

#include <string.h>
#include <vm.h>
#include <arch/gic.h>
#include <fences.h>
#include <arch/tlb.h>
#include <vmstack.h>
//#include <baoenclave.h>

enum { IPC_NOTIFY };

typedef union {
    struct {
        uint8_t shmem_id;
        uint8_t event_id;
    };
    uint64_t raw;
} ipc_msg_data_t;

static size_t shmem_table_size;
static shmem_t* shmem_table;
//struct config* config_ptr;

//uint64_t pos;

shmem_t* ipc_get_shmem(uint64_t shmem_id)
{
    if (shmem_id < shmem_table_size) {
        return &shmem_table[shmem_id];
    } else {
        return NULL;
    }
}

static ipc_t* ipc_find_by_shmemid(vm_t* vm, uint64_t shmem_id)
{
    ipc_t* ipc_obj = NULL;

    for (int i = 0; i < vm->ipc_num; i++) {
        if (vm->ipcs[i].shmem_id == shmem_id) {
            ipc_obj = &vm->ipcs[i];
            break;
        }
    }

    return ipc_obj;
}

static void ipc_notify(uint64_t shmem_id, uint64_t event_id)
{
    ipc_t* ipc_obj = ipc_find_by_shmemid(cpu.vcpu->vm, shmem_id);
    if (ipc_obj != NULL && event_id < ipc_obj->interrupt_num) {
        int irq_id = ipc_obj->interrupts[event_id];
        interrupts_vm_inject(cpu.vcpu, irq_id);
    }
}

static void ipc_handler(uint32_t event, uint64_t data)
{
    ipc_msg_data_t ipc_data = {.raw = data};
    switch (event) {
        case IPC_NOTIFY:
            ipc_notify(ipc_data.shmem_id, ipc_data.event_id);
            break;
    }
}
CPU_MSG_HANDLER(ipc_handler, IPC_CPUSMG_ID);

//void* alloc_baoenclave_struct(uint64_t physical_address, size_t size)
//{
//    ppages_t ppages = mem_ppages_get(physical_address, size);
//    void* va = mem_alloc_vpage(&cpu.as, SEC_HYP_GLOBAL, NULL, size);
//    if (mem_map(&cpu.as, va, &ppages, size, PTE_HYP_FLAGS)) {
//        ERROR("mem_map failed %s", __func__);
//    }
//
//    return va;
//}
//
//void alloc_baoenclave(void* vm_ptr, uint64_t donor_va)
//{
//    vm_t* vm = vm_ptr;
//    vm_config_t* vm_cfg = config_ptr->vmlist[0];
//
//    struct mem_region img_reg;
//    struct mem_region mem_reg;
//    struct mem_region shmem_reg;
//
//    uint64_t physical_address = 0;
//
//    uint64_t mem_pos = 0;
//
//    img_reg.phys = vm_cfg->image.load_addr;
//    img_reg.base = vm_cfg->image.base_addr;
//    img_reg.size = ALIGN(vm_cfg->image.size, PAGE_SIZE);
//    img_reg.place_phys = true;
//    img_reg.colors = 0;
//
//    vm_copy_img_to_rgn(vm_ptr, vm_cfg, &img_reg);
//    vm_map_mem_region(vm_ptr, &img_reg);
//    
//    mem_free_vpage(&cpu.vcpu->vm->as, (void *)donor_va, NUM_PAGES(config_ptr->config_size), false);
//
//
//    struct mem_region* reg = &vm_cfg->platform.regions[0];
//
//    /* we already mapped the image from base to image.size */
//    uintptr_t vm_mem_after_img = reg->base + vm_cfg->image.size;// + PAGE_SIZE;
//
//    /* leftover memory we need to map (discounting image size) */
//    uintptr_t leftover_to_map_vm = reg->size - vm_cfg->image.size;
//
//
//    /* could be optimized if(physical_address), guardando */
//    for (size_t i = 0; i < NUM_PAGES(leftover_to_map_vm); i++) {
//        mem_pos = (size_t)(donor_va + (i * PAGE_SIZE) + config_ptr->config_size);
//        mem_guest_ipa_translate((void*)mem_pos, &physical_address);
//
//
//        mem_reg.phys = physical_address;
//        mem_reg.base = vm_mem_after_img + (i * PAGE_SIZE);
//        mem_reg.size = PAGE_SIZE;
//        mem_reg.place_phys = true;
//        mem_reg.colors = 0;
//        vm_map_mem_region(vm, &mem_reg);
//        mem_free_vpage(&cpu.vcpu->vm->as, (void*)mem_pos, 1, false);
//    }
//    
//    uintptr_t vm_mem_after_img_mem = reg->base + reg->size;//uintptr_t vm_mem_after_img_mem = reg->base + PAGE_SIZE + reg->size;
//
//    for (size_t i = 0; i < NUM_PAGES(vm_cfg->platform.ipcs->size); i++) {
//        mem_pos = (size_t)(donor_va + (i * PAGE_SIZE) + PAGE_SIZE + reg->size);
//        mem_guest_ipa_translate((void*)mem_pos, &physical_address);
//        
//        
//        shmem_reg.phys = physical_address;
//        shmem_reg.base = vm_mem_after_img_mem + (i * PAGE_SIZE);
//        shmem_reg.size = PAGE_SIZE;
//        shmem_reg.place_phys = true;
//        shmem_reg.colors = 0;
//        vm_map_mem_region(vm, &shmem_reg);
//    }
//
//    tlb_vm_inv_all(cpu.vcpu->vm->id);
//}
//
//
//int64_t baoenclave_dynamic_hypercall(uint64_t id, uint64_t arg0, uint64_t arg1,
//                                     uint64_t arg2)
//{
//    int64_t res = HC_E_SUCCESS;
//    uint64_t physical_address = 0;
//    uint64_t physical_address2 = 0;
//
//    size_t c_size;
//
//    size_t full_size;
//
//    vcpu_t* child = NULL;
//
//    switch (id) {
//        case 0:
//            mem_guest_ipa_translate((void*)arg0, &physical_address);
//
//            /* One page*/
//            void* va = alloc_baoenclave_struct(physical_address, 1);
//            config_ptr = va;
//            c_size = config_ptr->config_size;
//
//            mem_free_vpage(&cpu.as, va, 1, true); //false para o linux, para testar é tentar traduzir um endereço dessa região
//
//            va = alloc_baoenclave_struct(physical_address, NUM_PAGES(c_size));
//            config_ptr = va;
//
//            config_adjust_to_va(config_ptr, physical_address);
//
//            vmm_init_dynamic(config_ptr, arg0);
//
//            break;
//        case 1:
//            /* optimizar com o adress space */
//            if((child = vcpu_get_child(cpu.vcpu, 0)) != NULL){
//                vmstack_push(child);
//            }
//            //child = vcpu_get_child(cpu.vcpu, 0);
//            
//            mem_guest_ipa_translate((void *)config_ptr->vmlist[0]->platform.regions->base, &physical_address2);
//    
//            vmstack_pop();
//            
//
//            full_size = config_ptr->vmlist[0]->platform.ipcs->size + config_ptr->vmlist[0]->platform.regions->size + config_ptr->config_header_size;
//
//
//            void* va2 = alloc_baoenclave_struct(physical_address2, NUM_PAGES(full_size));
//            memset(va2, 0, full_size);
//
//            mem_free_vpage(&cpu.as, va2, NUM_PAGES(full_size), true);
//            
//            if (mem_map(&cpu.vcpu->vm->as, (void *)arg0 + config_ptr->config_header_size, NULL, NUM_PAGES(full_size), PTE_VM_FLAGS)) {
//                ERROR("mem_map failed %s", __func__);
//            }
//            
//            INFO("Enclave Destroyed");
//            break;
//    }
//    return res;
//}

int64_t ipc_hypercall(uint64_t id, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    uint64_t ipc_id = arg0;
    uint64_t ipc_event = arg1;
    int64_t ret = -HC_E_SUCCESS;

    shmem_t* shmem = NULL;
    bool valid_ipc_obj = ipc_id < cpu.vcpu->vm->ipc_num;
    if (valid_ipc_obj) {
        shmem = ipc_get_shmem(cpu.vcpu->vm->ipcs[ipc_id].shmem_id);
    }
    bool valid_shmem = shmem != NULL;

    if (valid_ipc_obj && valid_shmem) {
        uint64_t ipc_cpu_masters = shmem->cpu_masters & ~cpu.vcpu->vm->cpus;

        ipc_msg_data_t data = {
            .shmem_id = cpu.vcpu->vm->ipcs[ipc_id].shmem_id,
            .event_id = ipc_event,
        };
        cpu_msg_t msg = {IPC_CPUSMG_ID, IPC_NOTIFY, data.raw};

        for (int i = 0; i < platform.cpu_num; i++) {
            if (ipc_cpu_masters & (1ULL << i)) {
                cpu_send_msg(i, &msg);
            }
        }

    } else {
        ret = -HC_E_INVAL_ARGS;
    }

    return ret;
}

static void ipc_alloc_shmem()
{
    for (int i = 0; i < shmem_table_size; i++) {
        shmem_t* shmem = &shmem_table[i];
        if (!shmem->place_phys) {
            size_t n_pg = NUM_PAGES(shmem->size);
            ppages_t ppages = mem_alloc_ppages(shmem->colors, n_pg, false);
            if (ppages.size < n_pg) {
                ERROR("failed to allocate shared memory");
            }
            shmem->phys = ppages.base;
        }
    }
}

static void ipc_setup_masters(const vm_config_t* vm_config, bool vm_master)
{
    static spinlock_t lock = SPINLOCK_INITVAL;

    for (int i = 0; i < vm_config_ptr->shmemlist_size; i++) {
        vm_config_ptr->shmemlist[i].cpu_masters = 0;
    }

    cpu_sync_barrier(&cpu_glb_sync);

    if (vm_master) {
        for (int i = 0; i < vm_config->platform.ipc_num; i++) {
            spin_lock(&lock);
            shmem_t* shmem =
                ipc_get_shmem(vm_config->platform.ipcs[i].shmem_id);
            if (shmem != NULL) {
                shmem->cpu_masters |= (1ULL << cpu.id);
            }
            spin_unlock(&lock);
        }
    }
}

void ipc_init(const vm_config_t* vm_config, bool vm_master)
{
    shmem_table_size = vm_config_ptr->shmemlist_size;
    shmem_table = vm_config_ptr->shmemlist;

    if (cpu.id == CPU_MASTER) {
        ipc_alloc_shmem();
    }

    ipc_setup_masters(vm_config, vm_master);
}
