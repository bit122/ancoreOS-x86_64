/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: storage.h
    Description: Storage module for the VNiX Operating System.
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


#ifndef STORAGE_DEVICE_H
#define STORAGE_DEVICE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "fs/filesystem.h"


// Drive struct.

typedef struct { 
    size_t port_base; 
    size_t mmio_base; 
    uint16_t flags;
} sata_charicteristics_t;


typedef struct { 
    uint8_t drive_number;
    uint16_t flags;
    uint16_t pmio_base; 
} pata_charicteristics_t;


typedef struct storage_device {
    /*
        First Character:
        S = SATA
        P or p = PATA (IDE)
        U = USB
        Q or q = ATAPI (CDROM)
        T = AHCI ATAPI (CDROM)
        N = NVMe
        E = eMMC
        F = FDD
        0xFF = reserved in case I add more (useful for uncommon devices)
    */
    char type_identifier;

    union {
        sata_charicteristics_t sata;  // For BOTH SATA AHCI Native Drive AND SATA AHCI ATAPI
        pata_charicteristics_t pata;  // For BOTH PATA IDE AND PATA ATAPI BOTH non-standard base port AND standard
        struct { int controller; } usb; // TODO: Implement
        struct { int pci_slot; } nvme;  // TODO: Implement
        // etc
    } device_data;

    uint32_t sector_size;
    uint64_t capacity;                // Last LBA
} storage_device_t;


typedef struct storage_component {
    storage_device_t storage_device;
    filesystem_list_t *filesystem_list;
    char device_name[64];
} storage_component_t;


// Disk read function type: lba, byte_count, buffer -> return 0 on success
typedef int (*disk_read_t)(storage_device_t *storage_dev, uint64_t lba, uint32_t byte_count, void *buffer);


int drive_read(storage_device_t *device, uint64_t lba, uint32_t sector_count, void *buf);
int list_directories(storage_device_t *device, filesystem_t *fs, disk_read_t read_disk, char *directory_path);


#endif // STORAGE_DEVICE_H