/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: vmm.c
    Description: The virtual memory manager for the VNiX Operating System.
    Author: Yazin Tantawi

    All components of the VNiX Operating System, except where otherwise noted, 
    are copyright of the Aspen Software Foundation (and the corresponding author(s)) and licensed under GPLv2 or later.
    For more information on the Gnu Public License Version 2, please refer to the LICENSE file
    or to the link provided here: https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html

 * THIS OPERATING SYSTEM IS PROVIDED "AS IS" AND "AS AVAILABLE" UNDER 
 * THE GNU GENERAL PUBLIC LICENSE VERSION 2, WITHOUT
 * WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE, AND NON-INFRINGEMENT.
 * 
 * TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, IN NO EVENT SHALL
 * THE AUTHORS, COPYRIGHT HOLDERS, OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE), ARISING IN ANY WAY OUT OF THE USE OF THIS OPERATING SYSTEM,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE OPERATING SYSTEM IS
 * WITH YOU. SHOULD THE OPERATING SYSTEM PROVE DEFECTIVE, YOU ASSUME THE COST OF
 * ALL NECESSARY SERVICING, REPAIR, OR CORRECTION.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF THE GNU GENERAL PUBLIC LICENSE
 * ALONG WITH THIS OPERATING SYSTEM; IF NOT, WRITE TO THE FREE SOFTWARE
 * FOUNDATION, INC., 51 FRANKLIN STREET, FIFTH FLOOR, BOSTON,
 * MA 02110-1301, USA.
*/
#define _KERNEL
#include <stdint.h>
#include "includes/pmm.h"
#include "includes/vmm.h"
#include "boot/limine.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "tools/includes/serial.h"
#include "tools/includes/log-info.h"


extern volatile struct limine_hhdm_request hhdm_request;

uint64_t hhdm_offset;
static uint64_t *kernel_pml4;

void *phys_to_virt(uint64_t phys) {
    return (void *)(phys + hhdm_offset);
}

 uint64_t virt_to_phys(void *virt) {
    return (uint64_t)virt - hhdm_offset;
}

void vmm_init(void) {
    uint64_t cr3;
    asm ("mov %%cr3, %0" : "=r"(cr3));
    cr3 &= ~0xFFF;              // clear flags

    //this just gets the hhdm offset from limine
    if (hhdm_request.response != NULL) {
        hhdm_offset = hhdm_request.response->offset;
    }
    
    kernel_pml4 = (uint64_t *)phys_to_virt(cr3);
    LOG_OK("VMM initialized successfully\n");
    SERIAL(Info, vmm_init, "VMM initialized successfully\n");
}

static uint64_t *alloc_table(void) {
    uint64_t phys = palloc();      // must return 4K-aligned frame
    memset(phys_to_virt(phys), 0, 4096);
    return phys_to_virt(phys);
}

void map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t *pml4 = kernel_pml4;
    
    // Extract user bit to propagate down
    uint64_t user_bit = flags & PTE_USER;

    uint64_t *pdpt;
    if (!(pml4[PML4_INDEX(virt)] & PTE_PRESENT)) {
        pdpt = alloc_table();
        pml4[PML4_INDEX(virt)] = virt_to_phys(pdpt) | PTE_PRESENT | PTE_WRITABLE | user_bit;
    } else {
        pdpt = phys_to_virt(pml4[PML4_INDEX(virt)] & ~0xFFF);
    }

    uint64_t *pd;
    if (!(pdpt[PDPT_INDEX(virt)] & PTE_PRESENT)) {
        pd = alloc_table();
        pdpt[PDPT_INDEX(virt)] = virt_to_phys(pd) | PTE_PRESENT | PTE_WRITABLE | user_bit;
    } else {
        pd = phys_to_virt(pdpt[PDPT_INDEX(virt)] & ~0xFFF);
    }

    uint64_t *pt;
    if (!(pd[PD_INDEX(virt)] & PTE_PRESENT)) {
        pt = alloc_table();
        pd[PD_INDEX(virt)] = virt_to_phys(pt) | PTE_PRESENT | PTE_WRITABLE | user_bit;
    } else {
        pt = phys_to_virt(pd[PD_INDEX(virt)] & ~0xFFF);
    }

    pt[PT_INDEX(virt)] = phys | flags | PTE_PRESENT;

    asm ("invlpg (%0)" :: "r"(virt) : "memory");
}

void unmap_page(uint64_t virt) {
    uint64_t *pml4 = kernel_pml4;

    if (!(pml4[PML4_INDEX(virt)] & PTE_PRESENT)) return;
    uint64_t *pdpt = phys_to_virt(pml4[PML4_INDEX(virt)] & ~0xFFF);

    if (!(pdpt[PDPT_INDEX(virt)] & PTE_PRESENT)) return;
    uint64_t *pd = phys_to_virt(pdpt[PDPT_INDEX(virt)] & ~0xFFF);

    if (!(pd[PD_INDEX(virt)] & PTE_PRESENT)) return;
    uint64_t *pt = phys_to_virt(pd[PD_INDEX(virt)] & ~0xFFF);

    pt[PT_INDEX(virt)] = 0;

    asm ("invlpg (%0)" :: "r"(virt) : "memory");
}

void phys_flush_cache(void *addr, uint64_t size)
{
    uintptr_t p = (uintptr_t)addr & ~63ULL;
    uintptr_t end = (uintptr_t)addr + size;

    while (p < end) {
        __asm__ volatile ("clflush (%0)" :: "r"(p));
        p += 64;
    }

    __asm__ volatile ("mfence");
}

void phys_invalidate_cache(void *addr, uint64_t size)
{
    uintptr_t p = (uintptr_t)addr & ~63ULL;
    uintptr_t end = (uintptr_t)addr + size;

    while (p < end) {
        __asm__ volatile ("clflush (%0)" :: "r"(p));
        p += 64;
    }

    __asm__ volatile ("mfence");
}
