/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: kernel.c
    Description: The UEFI kernel for the VNiX Operating System.
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

#include "boot/limine.h"
#include "terminal/src/flanterm_backends/fb.h"
#include "mm/heapalloc/tlsf.h"
#include <stdio.h>
#include "terminal/src/flanterm.h"
#include <string.h>
#include <stdlib.h>
#include "tools/includes/serial.h"
#include "arch/x86_64/includes/idt.h"
#include "arch/x86_64/includes/gdt.h"
#include "arch/x86_64/includes/isr.h"
#include "mm/includes/pmm.h"
#include "mm/includes/vmm.h"
#include "tools/includes/log-info.h"
#include "drivers/pic/includes/apic/apic.h"
#include "drivers/pic/includes/apic/apic_irq.h"
#include "shell/includes/keyboard.h"
#include "shell/includes/shell.h"
#include "drivers/storage/includes/stinit.h"
#include "drivers/hci/includes/ehci.h"
#include "drivers/pci/includes/pci.h"
#include <assert.h>


extern void syscall_init(void);

extern struct flanterm_context *global_flanterm;

static volatile struct limine_framebuffer_request fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

#define KERNEL_HEAP_SIZE 0x100000
static unsigned char kernel_heap[KERNEL_HEAP_SIZE] __attribute__((aligned(8)));
tlsf_t kernel_tlsf;



void init_heap() {
capture_boot_tsc();

    kernel_tlsf = tlsf_create_with_pool(kernel_heap, KERNEL_HEAP_SIZE);
    LOG_INFO("Heap initialized at %p\n", kernel_heap);
    SERIAL(Info, init_heap, "Heap initialized at %p\n", kernel_heap);
    
}

        void vmm_test_mapping(void) {
    uint64_t virt = 0x1000000000;  // random virtual address
    uint64_t phys = 0x200000;      // random physical address


    map_page(virt, phys, PTE_PRESENT | PTE_WRITABLE);

    // verify that the mapping works by accessing the virtual address
    uint64_t *ptr = (uint64_t *)virt;
    *ptr = 0x1234567890ABCDEF;  

    uint64_t value = *ptr;
    if (value == 0x1234567890ABCDEF) {
        LOG_OK("VMM test passed: Virtual address maps correctly!\n");
        SERIAL(Info, kernel_main, "VMM test passed: Virtual address maps correctly!\n");
        
    } else {
        LOG_FATAL("VMM test failed: Virtual address mapping is incorrect\n");
        SERIAL(Fatal, kernel_main, "VMM test failed: Virtual address mapping is incorrect\n");
        
    }
}

void vmm_test_unmap(void) {
    uint64_t virt = 0x1000000000;  // a virtual address to map
    uint64_t phys = 0x200000;     

    map_page(virt, phys, PTE_PRESENT | PTE_WRITABLE);

    //write value
    uint64_t *ptr = (uint64_t *)virt;
    *ptr = 0x1234567890ABCDEF; 

    //verifies the value
    uint64_t value = *ptr; 
    if (value == 0x1234567890ABCDEF) {
        LOG_OK("VMM test passed: Virtual address mapped and accessed correctly\n");
        SERIAL(Info, kernel_main, "VMM test passed: Virtual address maps and accessed correctly!\n");
        
    } else {
        LOG_FATAL("VMM test failed: Virtual address access failed before unmap\n");
        SERIAL(Fatal, kernel_main, "VMM test failed: Virtual address access failed before unmap\n");
        
    }

unmap_page(virt);

    //printf("Attempting to access unmapped page...\n");
    //ptr = (uint64_t *)virt;
    //*ptr = 0xDEADBEEF;  // this triggers a page fault. DO NOT UNCOMMENT THIS!!!!

}


void kernel_main(void) {
    struct limine_framebuffer *fb = fb_req.response->framebuffers[0];
    extern void enter_usermode(uint64_t entry, uint64_t stack, int argc, void* argv);
    global_flanterm = flanterm_fb_init(
        NULL,                    // malloc function
        NULL,                    // free function
        fb->address,             // framebuffer address
        fb->width,               // width
        fb->height,              // height
        fb->pitch,               // pitch
        fb->red_mask_size,       // red_mask_size
        fb->red_mask_shift,      // red_mask_shift
        fb->green_mask_size,     // green_mask_size
        fb->green_mask_shift,    // green_mask_shift
        fb->blue_mask_size,      // blue_mask_size
        fb->blue_mask_shift,     // blue_mask_shift
        NULL,                    // canvas
        NULL,                    // ansi_colours
        NULL,                    // ansi_bright_colours
        NULL,                    // background
        NULL,                    // background_bright
        NULL,                    // font
        NULL,                    // font_width
        NULL,                    // font_height
        0,                       // font_spacing
        0,                       // font_scale_x
        1,                       // font_scale_y
        0,                       // margin
        0,                       // margin_gradient
        0,                       // default_bg
        0xFFFFFF                 // default_fg (white text)
    );

    printf("%sWelcome to the%s %sVNiX Operating System!%s\n", COLOR_BOLD, COLOR_RESET,  COLOR_BOLD COLOR_CYAN, COLOR_RESET);
    printf("--------Kernel Specifications--------\n");
    printf("Kernel revision: 3A\n");
    printf("OS Version: 0.10.05\n");
    printf("Verbose kernel logging: TRUE\n");
    printf("Copyright (c) 2026 Aspen Software Foundation\n");
    printf("-------------------------------------\n");

    init_heap();
    GDT_Initialize();
    IDT_Initialize();
    vmm_init();
    pmm_init();
    ISR_Initialize();
    APIC_IRQ_Initialize();
    keyboard_apic_init();
    storage_init();
    enable_interrupts();
    start_pci_enumeration();
    shell_main();
    


    while (1);
}