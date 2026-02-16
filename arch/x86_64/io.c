/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: io.c
    Description: I/O module for the VNiX Operating System
    Author: Yazin Tantawi

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

#include <stdint.h>
#include "arch/x86_64/includes/io.h"
#include <stdio.h>
#include "tools/includes/serial.h" 
#include <stdbool.h> //for the "are_interrupts_enabled" function
#include "tools/includes/log-info.h"

void halt(void) {
    uint64_t rip;

    __asm__ volatile (
        "leaq (%%rip), %0"
        : "=r"(rip)
    );

   printf("[ EMERGENCY ] HALTED CPU AT INSTRUCTION: %llx\n", rip);
    serial_printf("[ EMERGENCY ] HALTED CPU AT INSTRUCTION: %llx\n", rip);
    // just disable interrupts and jump-halt
    __asm__ volatile (
        "cli\n"      // disable interrupts
        "1:\n"
        "hlt\n"      
        "jmp 1b\n"   
    );
}

void halt_interrupts_enabled() {
    uint64_t rip;

    __asm__ volatile (
        "leaq (%%rip), %0"
        : "=r"(rip)
    );

    printf("[ HALT ] Halted CPU at instruction: 0x%llx\n", rip);
        serial_printf("[ HALT ] Halted CPU at instruction: 0x%llx\n", rip);

    __asm__ volatile (
        "1:\n"
        "hlt\n"
        "jmp 1b\n"
    );
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}


bool are_interrupts_enabled(void) {
    uint64_t flags;
    __asm__ volatile("pushfq; pop %0" : "=r"(flags));
    return (flags & (1 << 9)) != 0;  // Bit 9 is IF
}

void enable_interrupts(void) {
    __asm__ volatile ("sti");
    
    if (are_interrupts_enabled()) {
        LOG_OK("Successfully enabled interrupts\n");
        SERIAL(Info, enable_interrupts, "Successfully enabled interrupts\n");
    } else {
        LOG_FATAL("Failed to enable interrupts\n");
        SERIAL(Info, enable_interrupts, "Failed to enable interrupts\n");
        
    }
}


void cpuid(uint32_t leaf, uint32_t subleaf,
                         uint32_t *eax, uint32_t *ebx,
                         uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx),
                       "=c"(*ecx), "=d"(*edx)
                     : "a"(leaf), "c"(subleaf));
}


void disable_interrupts(void) {
    __asm__ volatile ("cli");
    
    if (!are_interrupts_enabled()) {
        LOG_OK("Successfully disabled interrupts\n");
        SERIAL(Info, disable_interrupts, "Successfully enabled interrupts\n");
    } else {
        LOG_FATAL("Failed to disable interrupts\n");
        SERIAL(Info, disable_interrupts, "Failed to disable interrupts\n");
    }
}


