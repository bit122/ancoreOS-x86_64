/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: idt.c
    Description: IDT module for the VNiX Operating System
    Author: Mejd Almohammedi 

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

#include "arch/x86_64/includes/idt.h"
#include "arch/x86_64/includes/gdt.h"
#include <stdio.h>
#include <stdint.h>
#include "arch/x86_64/includes/io.h"
#include "tools/includes/log-info.h"

#define FLAG_SET(var, mask)   ((var) |= (mask))
#define FLAG_UNSET(var, mask) ((var) &= ~(mask))


typedef struct {
    uint16_t BaseLow;       // bits 0-15
    uint16_t SegmentSelector;
    uint8_t  IST;           // Interrupt Stack Table index (0-7)
    uint8_t  Flags;         // type/present bits
    uint16_t BaseMiddle;    // bits 16-31
    uint32_t BaseHigh;      // bits 32-63
    uint32_t Reserved;      // must be zero
} __attribute__((packed)) IDTEntry_t;


typedef struct {
    uint16_t Limit;
    uint64_t Ptr;
} __attribute__((packed)) IDTDescriptor_t;

IDTEntry_t g_IDT[256];

IDTDescriptor_t g_IDTDescriptor = { sizeof(g_IDT) - 1, (uint64_t)g_IDT };

void IDT_Load(IDTDescriptor_t *idtDescriptor) {
    __asm__ __volatile__ ("lidt (%0)" :: "r"(idtDescriptor));
}

void IDT_SetGate(int interrupt, void *base, uint16_t segmentDescriptor, uint8_t flags)
{
    uint64_t addr = (uint64_t)base;
    g_IDT[interrupt].BaseLow       = addr & 0xFFFF;
    g_IDT[interrupt].BaseMiddle    = (addr >> 16) & 0xFFFF;
    g_IDT[interrupt].BaseHigh      = (addr >> 32) & 0xFFFFFFFF;
    g_IDT[interrupt].SegmentSelector = segmentDescriptor;
    g_IDT[interrupt].IST           = 0;
    g_IDT[interrupt].Flags         = flags;
    g_IDT[interrupt].Reserved      = 0;
}


void IDT_EnableGate(int interrupt) {
    FLAG_SET(g_IDT[interrupt].Flags, IDT_FLAG_PRESENT);
}

void IDT_DisableGate(int interrupt) {
    FLAG_UNSET(g_IDT[interrupt].Flags, IDT_FLAG_PRESENT);
}

void IDT_Initialize() {
    IDT_Load(&g_IDTDescriptor);
        IDTDescriptor_t current;
    __asm__ volatile("sidt %0" : "=m"(current));


    if (current.Ptr == (uint64_t)g_IDTDescriptor.Ptr &&
        current.Limit == g_IDTDescriptor.Limit) {
        LOG_OK("IDT initialized successfully\n");
        SERIAL(Info, IDT_Initialize, "IDT initialized successfully\n");
        
    } else {
        LOG_FATAL("Failed to initialize IDT, halting...\n");
        SERIAL(Info, IDT_Initialize, "Failed to initialize IDT, halting...\n");
        halt(); 
    }

}
