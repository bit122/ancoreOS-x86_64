/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: pci.c
    Description: The PCI module for the VNiX Operating System.
    Author: Mejd Almohammedi, edited by Yazin Tantawi

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
#include <stdint.h>
#include <stdlib.h>
#include "includes/pci.h"
#include "arch/x86_64/includes/io.h"
#include "drivers/storage/includes/sata.h"
#include "kernel/time/includes/time.h"
#include "tools/includes/log-info.h"
#include "mm/includes/vmm.h"
#include "drivers/hci/includes/ehci.h"

#ifndef PTE_CACHE_DISABLE
#define PTE_CACHE_DISABLE   (1ULL << 4)  
#endif

#ifndef PTE_WRITE_THROUGH
#define PTE_WRITE_THROUGH   (1ULL << 3) 
#endif

#ifndef PTE_PRESENT
#define PTE_PRESENT         (1ULL << 0)   
#endif

uint32_t pid_rn = 0;

uint32_t pci_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    uint32_t address = (1U << 31)
                     | ((uint32_t)bus << 16)
                     | ((uint32_t)device << 11)
                     | ((uint32_t)func << 8)
                     | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t data) {
    uint32_t address = (1U << 31)
                     | ((uint32_t)bus << 16)
                     | ((uint32_t)device << 11)
                     | ((uint32_t)func << 8)
                     | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, data);
}


void scan_pci_device(uint8_t bus, uint8_t device) {
    uint32_t vendor_data = pci_read(bus, device, 0, 0x00);
    uint16_t vendor_id = vendor_data & 0xFFFF;
    if (vendor_id == 0xFFFF) return;

    uint32_t header_data = pci_read(bus, device, 0, 0x0C);
    uint8_t header_type = (header_data >> 16) & 0xFF;

    uint8_t func_limit = (header_type & 0x80) ? 8 : 1;

    for (uint8_t func = 0; func < func_limit; func++) {
        uint32_t vd = pci_read(bus, device, func, 0x00);
        uint16_t vid = vd & 0xFFFF;
        if (vid == 0xFFFF) continue;

        uint16_t did = (vd >> 16) & 0xFFFF;
        uint32_t class_data = pci_read(bus, device, func, 0x08);
        uint8_t class_code = (class_data >> 24) & 0xFF;
        uint8_t subclass = (class_data >> 16) & 0xFF;

        uint8_t prog_if    = (class_data >> 8) & 0xFF;

        if (class_code == 0x01) {
            const char* type = "Unknown Storage";

            if (subclass == 0x01) {
                type = "IDE";
            } else if (subclass == 0x04) {
                type = "RAID";
            } else if (subclass == 0x06) {
                if (prog_if == 0x01) {
                    type = "SATA AHCI";
                    uint32_t bar5 = pci_read(bus, device, func, 0x24);
                    uint32_t ahci_base = bar5 & 0xFFFFFFF0;
                    LOG_INFO("AHCI MMIO base address: %x\n", ahci_base);
                    SERIAL(Info, scan_pci_device, "AHCI MMIO base address: %x\n", ahci_base);
                    pid_rn = 7;
                    
                    // Map MMIO region with cache disabled for device memory
                    map_page(ahci_base, ahci_base, PTE_WRITABLE | PTE_CACHE_DISABLE | PTE_WRITE_THROUGH);
                    
                    sata_search(ahci_base);
                } else {
                    type = "SATA (non-AHCI)";
                }
            }

            
            LOG_INFO("%s Controller: bus=%u dev=%u func=%u vendor id=%x dev id=%x\n", type, bus, device, func, vid, did);
            SERIAL(Info, scan_pci_device, "%s Controller: bus=%u dev=%u func=%u vendor id=%x dev id=%x\n", type, bus, device, func, vid, did);
            
        } else if (class_code == 0x0C && subclass == 0x03) {
            // USB Controller
            const char* usb_type = "Unknown USB";
            
            if (prog_if == 0x00) {
                usb_type = "UHCI";
            } else if (prog_if == 0x10) {
                usb_type = "OHCI";
            } else if (prog_if == 0x20) {
                usb_type = "EHCI";
                LOG_INFO("EHCI Controller: bus=%u dev=%u func=%u vendor=%x dev=%x\n", bus, device, func, vid, did);
                SERIAL(Info, scan_pci_device, "EHCI Controller: bus=%u dev=%u func=%u vendor=%x dev=%x\n", bus, device, func, vid, did);
                ehci_pci_init(bus, device, func);
                
            } else if (prog_if == 0x30) {
                usb_type = "xHCI";
            }
            
            LOG_INFO("%s Controller: bus=%u dev=%u func=%u vendor=%x dev=%x\n", usb_type, bus, device, func, vid, did);
            SERIAL(Info, scan_pci_device, "%s Controller: bus=%u dev=%u func=%u vendor=%x dev=%x\n", usb_type, bus, device, func, vid, did);
                   
        } else if (class_code == 0x06 && subclass == 0x04) {
            uint32_t bus_data = pci_read(bus, device, func, 0x18);
            uint8_t secondary_bus = (bus_data >> 8) & 0xFF;
            scan_pci_bus(secondary_bus); // recurse
        }
    }
}


void scan_pci_bus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; device++) {
        scan_pci_device(bus, device);
    }
}


void start_pci_enumeration() {
    uint32_t header_data = pci_read(0, 0, 0, 0x0C);
    uint8_t header_type = (header_data >> 16) & 0xFF;

    if ((header_type & 0x80) == 0) {
        scan_pci_bus(0);
    } else {
        for (uint8_t func = 0; func < 8; func++) {
            uint32_t vd = pci_read(0, 0, func, 0x00);
            uint16_t vid = vd & 0xFFFF;
            if (vid == 0xFFFF) continue;
            uint32_t bus_data = pci_read(0, 0, func, 0x18);
            uint8_t bus_num = (bus_data >> 8) & 0xFF;
            scan_pci_bus(bus_num);
        }
    }
}

uint32_t pci_get_bar_size(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    uint32_t orig = pci_read(bus, device, func, offset);

    // write all 1's
    pci_write(bus, device, func, offset, 0xFFFFFFFF);

    // read back
    uint32_t size_mask = pci_read(bus, device, func, offset);

    // restore original
    pci_write(bus, device, func, offset, orig);

    // mask out low bits (type info)
    size_mask &= ~0xF;

    // size = ~(mask) + 1
    uint32_t size = (~size_mask) + 1;

    return size;
}

uint32_t pci_read_bar(uint8_t bus, uint8_t device, uint8_t func, uint8_t bar_num) {
    if (bar_num > 5) return 0; // PCI has 6 BARs (0-5)
    uint8_t offset = 0x10 + (bar_num * 4);
    return pci_read(bus, device, func, offset);
}