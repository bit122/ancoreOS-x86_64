/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: gdt.c
    Description: GDT module for the VNiX Operating System
    Author: Mejd Almohammedi, edited by Yazin Tantawi 

    All components of the VNiX Operating System, except where otherwise noted, 
    are copyright of the Aspen Software Foundation (and the corresponding author(s)) and licensed under GPLv2 or later.
    For more information on the GNU Public License Version 2, please refer to the LICENSE file
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

// Despite the lack of legal requirements, this file is licensed by Nanobyte under the unlicense:
// https://github.com/nanobyte-dev/nanobyte_os/blob/master/LICENSE

#include "arch/x86_64/includes/gdt.h"
#include "arch/x86_64/includes/idt.h"
#include <stdint.h>
#include "arch/x86_64/includes/io.h"
#include "tools/includes/log-info.h"
#include <string.h>

extern void syscall_handler_asm(void);

struct __attribute__((packed)) tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3; 
    uint64_t ist4; 
    uint64_t ist5; 
    uint64_t ist6; 
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} tss;



typedef struct {
    uint16_t LimitLow;                  // limit (bits 0-15)
    uint16_t BaseLow;                   // base (bits 0-15)
    uint8_t BaseMiddle;                 // base (bits 16-23)
    uint8_t Access;                     // access
    uint8_t FlagsLimitHi;               // limit (bits 16-19) | flags
    uint8_t BaseHigh;                   // base (bits 24-31)
} __attribute__((packed)) GDTEntry_t;

typedef struct {
    uint16_t Limit;                     // sizeof(gdt) - 1
    GDTEntry_t *Ptr;                    // address of GDT
} __attribute__((packed)) GDTDescriptor_t;

typedef enum{
    GDT_ACCESS_CODE_READABLE                = 0x02,
    GDT_ACCESS_DATA_WRITEABLE               = 0x02,

    GDT_ACCESS_CODE_CONFORMING              = 0x04,
    GDT_ACCESS_DATA_DIRECTION_NORMAL        = 0x00,
    GDT_ACCESS_DATA_DIRECTION_DOWN          = 0x04,

    GDT_ACCESS_DATA_SEGMENT                 = 0x10,
    GDT_ACCESS_CODE_SEGMENT                 = 0x18,

    GDT_ACCESS_DESCRIPTOR_TSS               = 0x00,

    GDT_ACCESS_RING0                        = 0x00,
    GDT_ACCESS_RING1                        = 0x20,
    GDT_ACCESS_RING2                        = 0x40,
    GDT_ACCESS_RING3                        = 0x60,

    GDT_ACCESS_PRESENT                      = 0x80,

} GDT_ACCESS_t;

typedef enum {
    GDT_FLAG_64BIT                          = 0x20,
    GDT_FLAG_32BIT                          = 0x40,
    GDT_FLAG_16BIT                          = 0x00,

    GDT_FLAG_GRANULARITY_1B                 = 0x00,
    GDT_FLAG_GRANULARITY_4K                 = 0x80,
} GDT_FLAGS_t;

// Helper macros
#define GDT_LIMIT_LOW(limit)                (limit & 0xFFFF)
#define GDT_BASE_LOW(base)                  (base & 0xFFFF)
#define GDT_BASE_MIDDLE(base)               ((base >> 16) & 0xFF)
#define GDT_FLAGS_LIMIT_HI(limit, flags)    (((limit >> 16) & 0xF) | (flags & 0xF0))
#define GDT_BASE_HIGH(base)                 ((base >> 24) & 0xFF)


#define GDT_ENTRY(base, limit, access, flags) {                     \
    GDT_LIMIT_LOW(limit),                                           \
    GDT_BASE_LOW(base),                                             \
    GDT_BASE_MIDDLE(base),                                          \
    access,                                                         \
    GDT_FLAGS_LIMIT_HI(limit, flags),                               \
    GDT_BASE_HIGH(base)                                             \
}

GDTEntry_t g_GDT[] = {
    // NULL descriptor
    GDT_ENTRY(0, 0, 0, 0),

    // kernel code (0x08)
    GDT_ENTRY(
        0,
        0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 |
        GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
        GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K
    ),

    // kernel data (0x10)
    GDT_ENTRY(
        0,
        0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 |
        GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE,
        GDT_FLAG_GRANULARITY_4K
    ),

    // user code (0x18 → 0x1B)
    GDT_ENTRY(
        0,
        0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 |
        GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
        GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K
    ),

    // user data (0x20 → 0x23)
    GDT_ENTRY(
        0,
        0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 |
        GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE,
        GDT_FLAG_GRANULARITY_4K
    ),

    GDT_ENTRY(
    0,  
    sizeof(tss) - 1,
    0x89,  
    0x00   
),

GDT_ENTRY(0, 0, 0, 0),

//it was that fkn easy???

};  




GDTDescriptor_t g_GDTDescriptor = { sizeof(g_GDT) - 1, g_GDT};

void GDT_Load(GDTDescriptor_t *descriptor, uint16_t cs, uint16_t ds)
{
    __asm__ __volatile__ (
        "lgdt (%0)\n"        // load GDT descriptor
        "movw %1, %%ds\n"    // reload data segments (16-bit)
        "movw %1, %%es\n"
        "movw %1, %%fs\n"
        "movw %1, %%gs\n"
        "movw %1, %%ss\n"
        "pushq %2\n"         // push 64-bit code segment for lretq
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"            // reload CS
        "1:\n"
        :: "r"(descriptor), "r"(ds), "r"((uint64_t)cs)
        : "rax"
    );
}

void TSS_Initialize() {
    memset(&tss, 0, sizeof(tss));
    tss.iopb_offset = sizeof(tss); //note to future self: this WILL need to point to a kernel stack 

    //get tss addr.
    uint64_t tss_base = (uint64_t)&tss;
    
    g_GDT[5].BaseLow = tss_base & 0xFFFF;
    g_GDT[5].BaseMiddle = (tss_base >> 16) & 0xFF;  // set the lower 32 bits of tss base in the first tss entry (index 5)
    g_GDT[5].BaseHigh = (tss_base >> 24) & 0xFF;
    uint32_t *tss_upper = (uint32_t *)&g_GDT[6];
    *tss_upper = (tss_base >> 32) & 0xFFFFFFFF;  // set upper 32 bits in the second tss entry at an index of 6
}


void GDT_Initialize() {
TSS_Initialize();
    GDT_Load(&g_GDTDescriptor, GDT_CODE_SEGMENT, GDT_DATA_SEGMENT);
    uint16_t cs, ds;
    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    __asm__ volatile ("mov %%ds, %0" : "=r"(ds));
    __asm__ volatile("ltr %0" :: "r"((uint16_t)0x28)); 

    if (cs == GDT_CODE_SEGMENT && ds == GDT_DATA_SEGMENT) {
        LOG_OK("GDT initialized successfully\n");
        SERIAL(Info, GDT_Initialize, "GDT initialized successfully\n");
    } else {
        LOG_FATAL("Failed to initialize GDT, halting...\n");
        SERIAL(Info, GDT_Initialize, "Failed to initalize GDT, halting...\n");
        halt();
    }
}

