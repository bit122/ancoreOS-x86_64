/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: apic.c
    Description: APIC module for the VNiX Operating System
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

#include <stdio.h>
#include "drivers/pic/includes/apic/apic.h"
#include "drivers/pic/includes/apic/apic_irq.h"
#include "arch/x86_64/includes/idt.h"
#include "arch/x86_64/includes/isr.h"
#include "arch/x86_64/includes/io.h"
#include "tools/includes/log-info.h"

volatile uint32_t* apic_base = (volatile uint32_t*)APIC_BASE;
volatile uint32_t* apic_io_base = (volatile uint32_t*)APIC_IO_BASE;
uint32_t local_apic_address;
uint32_t local_ioapic_address;
uint32_t apic_flags;
uint32_t *apic_entries;

// set the physical address for local APIC registers
void cpu_set_apic_base(uintptr_t apic) {
   uint32_t edx = 0;
   uint32_t eax = (apic & 0xFFFFF000) | IA32_APIC_BASE_MSR_ENABLE; // ensure address is 4KB aligned

#ifdef __PHYSICAL_MEMORY_EXTENSION__
   edx = (apic >> 32) & 0x0F; // handle 64-bit addressing
#endif

   cpuSetMSR(IA32_APIC_BASE_MSR, eax, edx);
}

// get the physical address of the APIC registers page
uintptr_t cpu_get_apic_base(void) {
   uint32_t eax, edx;
   cpuGetMSR(IA32_APIC_BASE_MSR, &eax, &edx);

#ifdef __PHYSICAL_MEMORY_EXTENSION__
   return (eax & 0xFFFFF000) | ((edx & 0x0F) << 32);
#else
   return (eax & 0xFFFFF000);
#endif
}

// configure IMCR to switch from PIC to APIC mode
void ConfigureIMCR(void) {
    uint8_t al = inb(0x22); // read IMCR register
    if (al & 0x80) { // check if IMCR is present
        outb(0x22, 0x70); // select IMCR register
        outb(0x23, 0x1);  // enable APIC mode
    }
}

// APIC initialization fn
void APIC_Initialize(void) {
    uint32_t eax, edx;

    uint64_t cr4 = ReadCR4();
    cr4 |= CR4_APIC_BIT;  // enable APIC (bit 9)
    WriteCR4(cr4);
    
    LOG_OK("APIC enabled in CR4\n");
    SERIAL(Info, APIC_Initialize, "APIC enabled in CR4\n");
    
    // get APIC base address from MSR
    cpuGetMSR(IA32_APIC_BASE_MSR, &eax, &edx);
    
    if (!(eax & IA32_APIC_BASE_MSR_ENABLE)) {
    
        eax |= IA32_APIC_BASE_MSR_ENABLE;
        cpuSetMSR(IA32_APIC_BASE_MSR, eax, edx);
        LOG_OK("Enabled APIC in MSR\n");
        SERIAL(Info, APIC_Initialize, "Enabled APIC in MSR\n");
    }
    
    // verify APIC is enabled
    cpuGetMSR(IA32_APIC_BASE_MSR, &eax, &edx);
    if (eax & IA32_APIC_BASE_MSR_ENABLE) {
        LOG_OK( "APIC enabled and verified in MSR\n");
        SERIAL(Info, APIC_Initialize, "APIC enabled and verified in MSR\n");
    } 
    else {
        LOG_FATAL("Failed to enable APIC in MSR!\n");
        SERIAL(Fatal, APIC_Initialize, "Failed to enable APIC in MSR!\n");
    }
    
    // initialize spurious interrupt vector
    APIC_Write(APIC_SVR, APIC_Read(APIC_SVR) | 0x100 | 0xFF);
    LOG_OK("APIC initialized successfully\n");
    SERIAL(Info, APIC_Initialize, "APIC initialized successfully\n");

}