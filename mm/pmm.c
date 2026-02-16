/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: pmm.c
    Description: The physical memory manager for the VNiX Operating System.
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
#define _KERNEL //for PAGE_SIZE in <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include "includes/pmm.h"
#include "boot/limine.h"
#include <stdio.h>
#include "tools/includes/log-info.h"
#include <limits.h>

static uint64_t pmm_total_pages = 0;
static uint64_t pmm_free_pages  = 0;


struct PhysicalMemoryRegion *free_mem_head = NULL;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 3};

__attribute__((used, section(".limine_requests"))) 
volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 3};

void pmm_init(void)
{

struct limine_memmap_response *response = memmap_request.response;
struct limine_hhdm_response *hhdm_response = hhdm_request.response; 
struct limine_memmap_entry **entries = response->entries;

    uint64_t entry_count = response->entry_count;

    for (uint64_t i = 0; i < entry_count; i++)
    {
        if (entries[i]->type == LIMINE_MEMMAP_USABLE)
        {
            uint64_t length = entries[i]->length;
            
            uint64_t base = ALIGN_UP(entries[i]->base, PAGE_SIZE);
            uint64_t end = ALIGN_DOWN((entries[i]->base + length), PAGE_SIZE);

            for (uint64_t current = base; current < end; current += PAGE_SIZE)
            {
                struct PhysicalMemoryRegion *region = (struct PhysicalMemoryRegion*)(current + hhdm_response->offset);
                region->base = current;
                region->next = free_mem_head;
                free_mem_head = region;
                pmm_total_pages++;
                pmm_free_pages++;
            }
        }
    }

    LOG_OK("PMM initialized successfully\n");
    SERIAL(Info, pmm_init, "PMM initialized successfully\n");
}

uint64_t palloc(void)
{
    if (!free_mem_head) return (uint64_t)NULL;
    
    struct PhysicalMemoryRegion *region = free_mem_head;
    free_mem_head = free_mem_head->next;
    pmm_free_pages--;
    return region->base;
}

void pfree(uint64_t physc_addr)
{

    struct limine_hhdm_response *hhdm_response = hhdm_request.response; 
    struct PhysicalMemoryRegion *region = (struct PhysicalMemoryRegion*)(physc_addr + hhdm_response->offset);
    region->next = free_mem_head;
    region->base = physc_addr;
    free_mem_head = region;
    pmm_free_pages++;
}

uint64_t pmm_get_total_pages(void)
{
    return pmm_total_pages;
}

uint64_t pmm_get_free_pages(void)
{
    return pmm_free_pages;
}

uint64_t pmm_get_used_pages(void)
{
    return pmm_total_pages - pmm_free_pages;
}
