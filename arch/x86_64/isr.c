/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: isr.c
    Description: Interrupt Service Routine module for the VNiX Operating System
    Author: Mejd Almohammedi

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

// Despite the lack of legal requirements, this file is licensed by Nanobyte under the unlicense:
// https://github.com/nanobyte-dev/nanobyte_os/blob/master/LICENSE

#include "arch/x86_64/includes/isr.h"
#include "arch/x86_64/includes/idt.h"
#include "arch/x86_64/includes/gdt.h"
#include "arch/x86_64/includes/io.h"
#include <stdio.h>
#include "tools/includes/serial.h"
#include <stddef.h>
#include "tools/includes/log-info.h"
#include "kernel/terminal/src/flanterm.h"

extern struct flanterm_context *global_flanterm;

ISRHandler_t g_ISRHandlers[256];

void ISR_InitializeGates();

void ISR_Initialize() {
    ISR_InitializeGates();

    ISR_RegisterHandler(14, page_fault_handler); 

    for (int i = 0; i < 256; i++) {
        if (i != 14) {  
            IDT_EnableGate(i);  
        }
    }

    LOG_OK("ISR initialized successfully\n");
    SERIAL(Info, ISR_Initialize, "ISR initialized successfully\n");
}

// yeah no im not touching any of this shit, i cannot go through another 8 hours of debugging
void ISR_Handler(Registers_t *regs) {

    if (g_ISRHandlers[regs->interrupt] != NULL) {
        g_ISRHandlers[regs->interrupt](regs);
    } else if (regs->interrupt >= 32) {
            printcol(COLOR_RED, "KERNEL PANIC!\n");
    serial_write("KERNEL PANIC!\n", 15);
        uint64_t cr2, cr3;
        asm("mov %%cr2, %0" : "=r"(cr2));
        asm("mov %%cr3, %0" : "=r"(cr3));
        
        printf("Unhandled interrupt %d!\n", regs->interrupt);
        serial_printf("Unhandled interrupt %d!\n", regs->interrupt);

        printf("  rax=%llx  rbx=%llx  rcx=%llx  rdx=%llx  rsi=%llx  rdi=%llx\n  r8=%llx  r9=%llx  r10=%llx  r11=%llx  r12=%llx  r13=%llx\n r14=%llx  r15=%llx\n  rsp=%llx  rbp=%llx  rip=%llx  rflags=%llx  cs=%llx  ss=%llx\n cr2=%llx  cr3=%llx\n  interrupt=%llx  errorcode=%llx\n",
               regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15, regs->rsp, regs->rbp, regs->rip, regs->rflags, regs->cs, regs->ss, cr2, cr3, regs->interrupt, regs->error);
        
        serial_printf("  rax=%llx  rbx=%llx  rcx=%llx  rdx=%llx  rsi=%llx  rdi=%llx\n  r8=%llx  r9=%llx  r10=%llx  r11=%llx  r12=%llx  r13=%llx\n r14=%llx  r15=%llx\n  rsp=%llx  rbp=%llx  rip=%llx  rflags=%llx  cs=%llx  ss=%llx\n cr2=%llx  cr3=%llx\n  interrupt=%llx  errorcode=%llx\n",
            regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15, regs->rsp, regs->rbp, regs->rip, regs->rflags, regs->cs, regs->ss, cr2, cr3, regs->interrupt, regs->error);

    } else {
        printcol(COLOR_RED, "KERNEL PANIC!\n");
    serial_write("KERNEL PANIC!\n", 15);
        uint64_t cr2, cr3;
        asm("mov %%cr2, %0" : "=r"(cr2));
        asm("mov %%cr3, %0" : "=r"(cr3));
        
        printf("Unhandled exception %d: %s\n", regs->interrupt, g_Exceptions[regs->interrupt]);
        serial_printf("Unhandled exception %d: %s\n", regs->interrupt, g_Exceptions[regs->interrupt]);
            printf("  rax=%llx  rbx=%llx  rcx=%llx  rdx=%llx  rsi=%llx  rdi=%llx\n  r8=%llx  r9=%llx  r10=%llx  r11=%llx  r12=%llx  r13=%llx\n r14=%llx  r15=%llx\n  rsp=%llx  rbp=%llx  rip=%llx  rflags=%llx  cs=%llx  ss=%llx\n cr2=%llx  cr3=%llx\n  interrupt=%llx  errorcode=%llx\n",
           regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15, regs->rsp, regs->rbp, regs->rip, regs->rflags, regs->cs, regs->ss, cr2, cr3, regs->interrupt, regs->error);
    
    serial_printf("  rax=%llx  rbx=%llx  rcx=%llx  rdx=%llx  rsi=%llx  rdi=%llx\n  r8=%llx  r9=%llx  r10=%llx  r11=%llx  r12=%llx  r13=%llx\n r14=%llx  r15=%llx\n  rsp=%llx  rbp=%llx  rip=%llx  rflags=%llx  cs=%llx  ss=%llx\n cr2=%llx  cr3=%llx\n  interrupt=%llx  errorcode=%llx\n",
        regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15, regs->rsp, regs->rbp, regs->rip, regs->rflags, regs->cs, regs->ss, cr2, cr3, regs->interrupt, regs->error);

        halt();
    }
}

void page_fault_handler(Registers_t *regs) {
    uint64_t cr2, cr3;
    asm("mov %%cr2, %0" : "=r"(cr2));
    asm("mov %%cr3, %0" : "=r"(cr3));
    
    printcol(COLOR_RED, "KERNEL PANIC!\n");
    serial_write("KERNEL PANIC!\n", 15);
    
    printcol(COLOR_LIGHTRED, "PAGE FAULT TRIGGERED!\n");
    printcol(COLOR_BOLD, "You are most likely trying to access an invalid or non-mapped memory address.\n");
   printcol(COLOR_BOLD, "Please consult the documentation or external resources for more information on proper memory handling.\n");
    
    printf("Error code: %llx\n", regs->error);
    printf("  Faulting address (CR2): %llx\n", cr2);
    printf("  Page table base (CR3): %llx\n", cr3);

    serial_printf("PAGE FAULT TRIGGERED!\n");
    serial_printf("You are most likely trying to access an invalid or non-mapped memory address.\n");
    serial_printf("Please consult the documentation or external resources for more information on proper memory handling.\n");
    serial_printf("Error code: %llx\n", regs->error);
    serial_printf("  Faulting address (CR2): %llx\n", cr2);
    serial_printf("  Page table base (CR3): %llx\n", cr3);

    printf("  rax=%llx  rbx=%llx  rcx=%llx  rdx=%llx  rsi=%llx  rdi=%llx\n  r8=%llx  r9=%llx  r10=%llx  r11=%llx  r12=%llx  r13=%llx\n r14=%llx  r15=%llx\n  rsp=%llx  rbp=%llx  rip=%llx  rflags=%llx  cs=%llx  ss=%llx\n cr2=%llx  cr3=%llx\n  interrupt=%llx  errorcode=%llx\n",
           regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15, regs->rsp, regs->rbp, regs->rip, regs->rflags, regs->cs, regs->ss, cr2, cr3, regs->interrupt, regs->error);
    
    serial_printf("  rax=%llx  rbx=%llx  rcx=%llx  rdx=%llx  rsi=%llx  rdi=%llx\n  r8=%llx  r9=%llx  r10=%llx  r11=%llx  r12=%llx  r13=%llx\n r14=%llx  r15=%llx\n  rsp=%llx  rbp=%llx  rip=%llx  rflags=%llx  cs=%llx  ss=%llx\n cr2=%llx  cr3=%llx\n  interrupt=%llx  errorcode=%llx\n",
        regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15, regs->rsp, regs->rbp, regs->rip, regs->rflags, regs->cs, regs->ss, cr2, cr3, regs->interrupt, regs->error);

    if (regs->error & 0x1) {
        printf( "Page fault caused by invalid read operation.\n");
        serial_write("Page fault caused by invalid read operation.\n", 46);
    } else {
        printf("Page fault caused by invalid write operation.\n");
        serial_write("Page fault caused by invalid write operation.\n", 47);
    }

    halt();
}

void ISR_RegisterHandler(int interrupt, ISRHandler_t handler) {
    g_ISRHandlers[interrupt] = handler;
    IDT_EnableGate(interrupt);
}

void panic(Registers_t *regs) {
        uint64_t cr2, cr3;
    asm("mov %%cr2, %0" : "=r"(cr2));
    asm("mov %%cr3, %0" : "=r"(cr3));
    printf("  rax=%llx  rbx=%llx  rcx=%llx  rdx=%llx  rsi=%llx  rdi=%llx\n  r8=%llx  r9=%llx  r10=%llx  r11=%llx  r12=%llx  r13=%llx\n r14=%llx  r15=%llx\n  rsp=%llx  rbp=%llx  rip=%llx  rflags=%llx  cs=%llx  ss=%llx\n cr2=%llx  cr3=%llx\n  interrupt=%llx  errorcode=%llx\n",
           regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15, regs->rsp, regs->rbp, regs->rip, regs->rflags, regs->cs, regs->ss, cr2, cr3, regs->interrupt, regs->error);
    
    serial_printf("  rax=%llx  rbx=%llx  rcx=%llx  rdx=%llx  rsi=%llx  rdi=%llx\n  r8=%llx  r9=%llx  r10=%llx  r11=%llx  r12=%llx  r13=%llx\n r14=%llx  r15=%llx\n  rsp=%llx  rbp=%llx  rip=%llx  rflags=%llx  cs=%llx  ss=%llx\n cr2=%llx  cr3=%llx\n  interrupt=%llx  errorcode=%llx\n",
        regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15, regs->rsp, regs->rbp, regs->rip, regs->rflags, regs->cs, regs->ss, cr2, cr3, regs->interrupt, regs->error);

    halt();
}