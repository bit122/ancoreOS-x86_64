/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: atapi.c
    Description: ATAPI module for the VNiX Operating System.
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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "arch/x86_64/includes/io.h"
#include "includes/ata.h"
#include "includes/atapi.h"
#include "includes/scsi.h"
#include "tools/includes/endian.h"


// Read status + error globally
uint8_t atapi_status = 0;
uint8_t atapi_error  = 0;



int atapi_read_sector(uint32_t lba, uint16_t sector_count, void *buffer, uint8_t drive, uint32_t sector_size) {
    uint16_t io_base;
    uint8_t drive_head;
    ata_get_io_and_head(drive, &io_base, &drive_head);
    outb(io_base + ATA_REG_DRIVE_HEAD, drive_head);
    if (ata_wait_ready(io_base, 0) != 0) return -1;

    // Setup transfer length (2048 bytes = 0x0800)
    outb(io_base + ATA_REG_FEATURES, 0);
    outb(io_base + ATA_REG_SECCOUNT, 0);
    outb(io_base + ATA_REG_LBA_LOW, 0);
    outb(io_base + ATA_REG_LBA_MID, 0);
    outb(io_base + ATA_REG_LBA_HIGH, 8);
    outb(io_base + ATA_REG_COMMAND, ATAPI_CMD_PACKET);

    // Wait for DRQ (device ready to receive packet)
    if (ata_wait_ready(io_base, 1) != 0) {
        ata_status = inb(io_base + ATA_REG_STATUS);
        ata_error  = inb(io_base + ATA_REG_ERROR);
        return -2;
    }

    // Build 12-byte READ(10) packet
    uint8_t packet[12] = {0};
    build_read10_atapi_cmd(packet, lba, sector_count);
    

    // Send packet (6 words)
    outsw(io_base + ATA_REG_DATA, packet, 6);

    // Read each sector
    uint16_t *buf = (uint16_t*)buffer;
    for (int s = 0; s < sector_count; s++) {
        if (ata_wait_ready(io_base, 1) != 0) return -3;

        // Read 2048 bytes (sector_size) per sector
        for (uint32_t i = 0; i < (sector_size / 2); i++) {
            buf[s * (sector_size / 2) + i] = inw(io_base + ATA_REG_DATA);
        }
    }

    return 0;
}

int atapi_read_sector_manual(uint32_t lba, uint16_t sector_count, void *buffer, uint8_t drive, uint16_t pmio_base, uint32_t sector_size) {
    uint16_t io_base;
    uint8_t drive_head;
    ata_get_io_and_head(drive, &io_base, &drive_head);
    io_base = pmio_base;
    outb(io_base + ATA_REG_DRIVE_HEAD, drive_head);
    if (ata_wait_ready(io_base, 0) != 0) return -1;

    // Setup transfer length (2048 bytes = 0x0800)
    outb(io_base + ATA_REG_FEATURES, 0);
    outb(io_base + ATA_REG_SECCOUNT, 0);
    outb(io_base + ATA_REG_LBA_LOW, 0);
    outb(io_base + ATA_REG_LBA_MID, 0);
    outb(io_base + ATA_REG_LBA_HIGH, 8);
    outb(io_base + ATA_REG_COMMAND, ATAPI_CMD_PACKET);

    // Wait for DRQ (device ready to receive packet)
    if (ata_wait_ready(io_base, 1) != 0) {
        ata_status = inb(io_base + ATA_REG_STATUS);
        ata_error  = inb(io_base + ATA_REG_ERROR);
        return -2;
    }

    // Build 12-byte READ(10) packet
    uint8_t packet[12] = {0};
    build_read10_atapi_cmd(packet, lba, sector_count);

    // Send packet (6 words)
    outsw(io_base + ATA_REG_DATA, packet, 6);

    // Read each sector
    uint16_t *buf = (uint16_t*)buffer;
    for (int s = 0; s < sector_count; s++) {
        if (ata_wait_ready(io_base, 1) != 0) return -3;

        // Read 2048 bytes (sector_size) per sector
        for (uint32_t i = 0; i < (sector_size / 2); i++) {
            buf[s * (sector_size / 2) + i] = inw(io_base + ATA_REG_DATA);
        }
    }

    return 0;
}

// ATA I/O wait - 400ns delay by reading alternate status port 4 times
static inline void ata_io_wait(uint16_t io_base) {
    for (int i = 0; i < 4; i++) {
        (void)inb(io_base + 0x206); // Alternate Status register is usually at I/O base + 0x206
    }
}

// Identify ATAPI device (CD/DVD drives)
int identify_atapi(uint8_t drive, uint16_t identify_data[256]) {
    uint16_t io_base;
    uint8_t drive_head;
    ata_get_io_and_head(drive, &io_base, &drive_head);

    outb(io_base + ATA_REG_DRIVE_HEAD, drive_head);
    ata_io_wait(io_base);

    outb(io_base + ATA_REG_FEATURES, 0);
    outb(io_base + ATA_REG_SECCOUNT, 0);
    outb(io_base + ATA_REG_LBA_LOW, 0);
    outb(io_base + ATA_REG_LBA_MID, 0);
    outb(io_base + ATA_REG_LBA_HIGH, 0);
    outb(io_base + ATA_REG_COMMAND, ATAPI_CMD_IDENTIFY);

    if (ata_wait_ready(io_base, 1) != 0) return -1;

    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(io_base + ATA_REG_DATA);
    }

    // Optional signature checks could go here

    return 0;
}


int identify_atapi_manual(uint8_t drive, uint16_t identify_data[256], uint16_t pmio_base)  {
    uint16_t io_base;
    uint8_t drive_head;
    ata_get_io_and_head(drive, &io_base, &drive_head);
    io_base = pmio_base;

    outb(io_base + ATA_REG_DRIVE_HEAD, drive_head);
    ata_io_wait(io_base);

    outb(io_base + ATA_REG_FEATURES, 0);
    outb(io_base + ATA_REG_SECCOUNT, 0);
    outb(io_base + ATA_REG_LBA_LOW, 0);
    outb(io_base + ATA_REG_LBA_MID, 0);
    outb(io_base + ATA_REG_LBA_HIGH, 0);
    outb(io_base + ATA_REG_COMMAND, ATAPI_CMD_IDENTIFY);

    if (ata_wait_ready(io_base, 1) != 0) return -1;

    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(io_base + ATA_REG_DATA);
    }

    // Optional signature checks could go here

    return 0;
}


int identify_atapi_useful(read10_capabillity_buffer_t *buffer, uint8_t drive) {
    uint16_t io_base;
    uint8_t drive_head;
    ata_get_io_and_head(drive, &io_base, &drive_head);
    outb(io_base + ATA_REG_DRIVE_HEAD, drive_head);
    if (ata_wait_ready(io_base, 0) != 0) return -1;

    outb(io_base + ATA_REG_FEATURES, 0);
    outb(io_base + ATA_REG_SECCOUNT, 0);  // Should be 0 for ATAPI PACKET command
    outb(io_base + ATA_REG_LBA_LOW,  0);
    outb(io_base + ATA_REG_LBA_MID,  (sizeof(read10_capabillity_buffer_t)) & 0xFF);     // Set expected byte count low (8 bytes)
    outb(io_base + ATA_REG_LBA_HIGH, (sizeof(read10_capabillity_buffer_t)>>8) &0xFF);   // Set expected byte count high
    outb(io_base + ATA_REG_COMMAND,  ATAPI_CMD_PACKET);

    // Wait for DRQ (device ready to receive packet)
    if (ata_wait_ready(io_base, 1) != 0) {
        ata_status = inb(io_base + ATA_REG_STATUS);
        ata_error  = inb(io_base + ATA_REG_ERROR);
        return -2;
    }

    // Build 12-byte READ CAPACITY(10) packet
    uint8_t packet[12] = {0};
    build_read_capacity_atapi_cmd(packet);

    // Send packet (6 words)
    outsw(io_base + ATA_REG_DATA, packet, 6);

    // Wait for data to be ready before reading
    if (ata_wait_ready(io_base, 1) != 0) {
        return -3;
    }

    // Read 8 bytes (4 words) of response data
    uint16_t *buf = (uint16_t *)buffer;
    for (size_t i = 0; i < (size_t)sizeof(read10_capabillity_buffer_t) / 2; i++) {
        buf[i] = inw(io_base + ATA_REG_DATA);
    }


    read10_capabillity_buffer_t *buf32 = (read10_capabillity_buffer_t *)buffer;
    buf32->sector_size = be32toh(buf32->sector_size);
    buf32->last_lba = be32toh(buf32->last_lba);

    return 0;
}


int identify_atapi_useful_manual(read10_capabillity_buffer_t *buffer, uint8_t drive, uint16_t pmio_base) {
    uint16_t io_base;
    uint8_t drive_head;
    ata_get_io_and_head(drive, &io_base, &drive_head);
    io_base = pmio_base;
    outb(io_base + ATA_REG_DRIVE_HEAD, drive_head);
    if (ata_wait_ready(io_base, 0) != 0) return -1;


    outb(io_base + ATA_REG_FEATURES, 0);
    outb(io_base + ATA_REG_SECCOUNT, 0);  // Should be 0 for ATAPI PACKET command
    outb(io_base + ATA_REG_LBA_LOW,  0);
    outb(io_base + ATA_REG_LBA_MID,  (uint8_t)((sizeof(read10_capabillity_buffer_t)) & 0xFF));     // Set expected byte count low (8 bytes)
    outb(io_base + ATA_REG_LBA_HIGH, (uint8_t)((sizeof(read10_capabillity_buffer_t)>>8) &0xFF));   // Set expected byte count high
    outb(io_base + ATA_REG_COMMAND,  ATAPI_CMD_PACKET);


    // Wait for DRQ (device ready to receive packet)
    if (ata_wait_ready(io_base, 1) != 0) {
        ata_status = inb(io_base + ATA_REG_STATUS);
        ata_error  = inb(io_base + ATA_REG_ERROR);
        return -2;
    }

    // Build 12-byte READ CAPACITY(10) packet
    uint8_t packet[12] = {0};
    build_read_capacity_atapi_cmd(packet);

    // Send packet (6 words)
    outsw(io_base + ATA_REG_DATA, packet, 6);

    // Read each sector
    uint16_t *buf = (uint16_t *)buffer;
    for (size_t i = 0; i < (size_t)sizeof(read10_capabillity_buffer_t) / 2; i++) {
        buf[i] = inw(io_base + ATA_REG_DATA);
    }

    read10_capabillity_buffer_t *buf32 = (read10_capabillity_buffer_t *)buffer;
    buf32->sector_size = be32toh(buf32->sector_size);
    buf32->last_lba = be32toh(buf32->last_lba);

    return 0;
}