/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: apic_irq.c
    Description: APIC IRQ module for the VNiX Operating System
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

#include "drivers/pic/includes/apic/apic.h"
#include "drivers/pic/includes/apic/apic_irq.h"
#include "drivers/pic/includes/pic_irq.h"
#include "arch/x86_64/includes/io.h"
#include "arch/x86_64/includes/isr.h"

#include <stddef.h>
#include <stdio.h>
#include "tools/includes/log-info.h"
#include "tools/includes/serial.h"

#define APIC_REMAP_OFFSET        0x20  // remap base for APIC interrupts
#define MAX_IRQS                 64    // set for simplicity (i can NOT debug for more than 30 minutes)

// APIC register Offsets
#define APIC_ISR  0x100  
#define APIC_IRR  0x200 

// global IRQ Handlers
IRQHandler_t g_APICIRQHandler_ts[MAX_IRQS];

// interrupt handler for APIC IRQs
void APIC_IRQ_Handler(Registers_t* regs) {
    int irq = regs->interrupt - APIC_REMAP_OFFSET;

    // read the IRR (Interrupt Request Register) and ISR (In-Service Register) from the APIC
    uint32_t apic_isr = APIC_Read(APIC_ISR);  
    uint32_t apic_irr = APIC_Read(APIC_IRR);  

    // check if the IRQ has a registered handler
    if (g_APICIRQHandler_ts[irq] != NULL) {
        // call the handler for the specific interrupt
        g_APICIRQHandler_ts[irq](regs);
    } else {
        LOG_WARN("Unhandled APIC IRQ %d  ISR=%x  IRR=%x...\n", irq, apic_isr, apic_irr);
        SERIAL(Warn, APIC_IRQ_Handler, "Unhandled APIC IRQ %d  ISR=%x  IRR=%x...\n", irq, apic_isr, apic_irr);
    }

    // send EOI to the APIC
    LAPIC_SendEOI();
}

void APIC_IRQ_Initialize() {
    APIC_Initialize();

    
    // register the IRQ handler for APIC IRQs
    for (int i = 0; i < MAX_IRQS; i++) {
        ISR_RegisterHandler(APIC_REMAP_OFFSET + i, APIC_IRQ_Handler);
    }

    LOG_OK("APIC IRQ initialized successfully\n");
    SERIAL(Info, APIC_IRQ_Initialize, "APIC IRQ initialized successfully\n");
}

// register an IRQ handler for a specific APIC IRQ
void APIC_IRQ_RegisterHandler(uint32_t irq, IRQHandler_t handler) {
    if (irq < 0 || irq >= MAX_IRQS) {
        printf("Invalid IRQ number %d\n", irq);
        serial_write("Invalid IRQ number\n", 22);
        return;
    }

    g_APICIRQHandler_ts[irq] = handler;
}

// set up a specific LVT entry (e.g., LINT0 or Timer) to enable interrupts
void APIC_EnableIRQ(uint32_t irq) {
    uint32_t value = APIC_Read(APIC_LVT_TIMER + irq * APIC_REGISTER_OFFSET);
    value &= ~0x10000; // clear the mask bit to unmask the interrupt
    APIC_Write(APIC_LVT_TIMER + irq * APIC_REGISTER_OFFSET, value);
}

// disable a specific IRQ
void APIC_DisableIRQ(uint32_t irq) {
    uint32_t value = APIC_Read(APIC_LVT_TIMER + irq * APIC_REGISTER_OFFSET);
    value |= 0x10000; // set the mask bit to mask the interrupt
    APIC_Write(APIC_LVT_TIMER + irq * APIC_REGISTER_OFFSET, value);
}

  
// enable I/O APIC interrupt (this is just a write to the redirection table entry)
void APIC_EnableIOIRQ(uint32_t irq, uint32_t interrupt_destination) {
    uint32_t redirection_entry = irq * APIC_REGISTER_OFFSET;

    // set delivery mode to fixed, vector to the keyboard interrupt vector, unmask the interrupt
    uint32_t value = (interrupt_destination | APIC_DELIVERY_MODE_FIXED);
    value &= ~0x10000;  // unmask interrupt (clear the mask bit)

    APIC_WriteIO(redirection_entry, value);
}

// disable I/O APIC interrupt
void APIC_DisableIOIRQ(uint32_t irq) {
    uint32_t redirection_entry = irq * APIC_REGISTER_OFFSET;

    // mask the interrupt by setting the mask bit
    uint32_t value = APIC_ReadIO(redirection_entry);
    value |= 0x10000;  
    APIC_WriteIO(redirection_entry, value);
}