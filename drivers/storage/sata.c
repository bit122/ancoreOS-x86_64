/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: sata.c
    Description: SATA module for the VNiX Operating System.
    Author: Mejd Almohammedi. edited by Yazin Tantawi

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
#include <stdlib.h>
#include <stdio.h>
#include "arch/x86_64/includes/io.h"
#include "includes/sata.h"
#include "includes/scsi.h"
#include "tools/includes/endian.h"
#include "tools/includes/log-info.h"
#include "mm/includes/vmm.h"
#include "tools/includes/endian.h"
#include "kernel/time/includes/time.h"
#include "includes/stinit.h" 

extern uint32_t pid_rn;

void sata_search(uint32_t mmio_base) {
    uint32_t pi = *(volatile uint32_t *)(mmio_base + 0x0C);
    for (int p = 0; p < 32; p++) {
        if (!(pi & (1 << p))) continue;

        volatile uint32_t *port_base = (volatile uint32_t *)(mmio_base + 0x100 + p * 0x80);
        uint32_t ssts = port_base[0x28 / 4];
        uint8_t det = ssts & 0x0F;
        uint8_t ipm = (ssts >> 8) & 0x0F;
        if (det != 3 || ipm != 1) continue;

        uint32_t sig = port_base[0x24 / 4];
        switch (sig) {
            case 0x00000101: {
                LOG_INFO("Port %d: SATA drive detected\n", p);
                SERIAL(Info, sata_search, "SATA drive detected");
                void *identify_buffer = malloc(512);
                sata_ahci_identify(port_base, identify_buffer);

                size_t sector_size = ((uint16_t *)identify_buffer)[117] ? (size_t)((uint16_t *)identify_buffer)[117] : 512;
                void *read_buffer = malloc(ALIGN_UP((size_t)600, sector_size));

                sata_ahci_read_sector(port_base, 0, (uint16_t)(ALIGN_UP((size_t)600, sector_size)/sector_size), read_buffer);
                LOG_INFO("GUID: 0x%x\n", ((uint32_t *)read_buffer)[0x8E]);
                SERIAL(Info, sata_search, "Read sector successful");
                free(read_buffer);
                free(identify_buffer);
                break;
            } case 0xEB140101: {
                LOG_INFO("Port %d: ATAPI drive detected\n", p);
                SERIAL(Info, sata_search, "ATAPI drive detected");
                void *read_buffer = malloc(2048);

                sata_ahci_read_sector_satapi(port_base, 0, 1, read_buffer);
                LOG_INFO("GUID: 0x%x\n", ((uint32_t *)read_buffer)[0x8E]);
                SERIAL(Info, sata_search, "ATAPI read successful");
                free(read_buffer);
                break;
            } case 0x96690101:
                LOG_INFO("Port %d: Enclosure management bridge\n", p);
                SERIAL(Info, sata_search, "Enclosure management bridge detected");
                break;
            case 0xC33C0101:
                LOG_INFO("Port %d: Port multiplier\n", p);
                SERIAL(Info, sata_search, "Port multiplier detected");
                break;
            default:
                LOG_WARN("Port %d: Unknown device type: 0x%x\n", p, sig);
                SERIAL(Warning, sata_search, "Unknown device type detected");
                break;
        }
    }
}

int sata_ahci_read_sector(volatile uint32_t *port_base, uint64_t lba, uint16_t sector_count, void *buffer) {
    SERIAL(Info, sata_ahci_read_sector, "Starting SATA sector read");
    ahci_port_mem_t *port_mem = malloc(sizeof(ahci_port_mem_t));
    memset(port_mem, 0, sizeof(ahci_port_mem_t));

    uint64_t cl_addr = (uint64_t)(size_t)(port_mem->cl);
    port_base[0x00 / 4] = (uint32_t)(cl_addr);
    port_base[0x04 / 4] = (uint32_t)(cl_addr >> 32);

    uint64_t fb_addr = (uint64_t)(size_t)(port_mem->rfis);
    port_base[0x08 / 4] = (uint32_t)(fb_addr);
    port_base[0x0C / 4] = (uint32_t)(fb_addr >> 32);

    port_base[0x18 / 4] &= ~0x01;     // Clear ST bit
    port_base[0x18 / 4] &= ~(1 << 4); // Clear FRE bit

    unsigned int timeout = SATA_WAIT_TIMEOUT;
    while ((port_base[0x18 / 4] & (1 << 15)) && timeout--) {
        if (timeout == 0) {
            LOG_WARN("Timeout waiting for CR clear\n");
            SERIAL(Warning, sata_ahci_read_sector, "Timeout waiting for CR clear");
            free(port_mem);
            return 6;
        }
        udelay(10);
    }

    timeout = SATA_WAIT_TIMEOUT;
    while ((port_base[0x18 / 4] & (1 << 14)) && timeout--) {
        if (timeout == 0) {
            LOG_WARN("Timeout waiting for FR clear\n");
            SERIAL(Warning, sata_ahci_read_sector, "Timeout waiting for FR clear");
            free(port_mem);
            return 7;
        }
        udelay(10);
    }

    hba_cmd_header_t *cmd_header = (hba_cmd_header_t *)(port_mem->cl);
    memset(&cmd_header[0], 0, sizeof(hba_cmd_header_t));
    // Calculate how many PRDT entries we need (max 8KB per entry)
    // 8KB = 16 sectors, so divide sector_count by 16 and round up
    int prdt_entries = (sector_count + 15) / 16;
    if (prdt_entries > 65535) prdt_entries = 65535; // limit if needed

    cmd_header[0].flags = 5;
    cmd_header[0].prdt_length = prdt_entries;

    hba_cmd_tbl_t *cmd_tbl = (hba_cmd_tbl_t *)(port_mem->ct);
    memset(cmd_tbl, 0, sizeof(hba_cmd_tbl_t));
    uint64_t ct_addr = (uint64_t)(size_t)(cmd_tbl);
    cmd_header[0].ctba = (uint32_t)(ct_addr);
    cmd_header[0].ctbau = (uint32_t)(ct_addr >> 32);

    // Fill PRDT entries
    uint8_t *buf = (uint8_t *)buffer;
    int remaining_sectors = sector_count;
    int i = 0;

    for (; i < prdt_entries - 1; i++) {
        cmd_tbl->prdt_entry[i].dba = (uint32_t)((uint64_t)(size_t)buf);
        cmd_tbl->prdt_entry[i].dbau = (uint32_t)(((uint64_t)(size_t)buf) >> 32);
        cmd_tbl->prdt_entry[i].dbc = (8 * 1024) - 1; // 8KB per PRDT entry minus 1
        cmd_tbl->prdt_entry[i].i = 1;

        buf += 8 * 1024;         // advance buffer by 8KB
        remaining_sectors -= 16; // 16 sectors (16 * 512 = 8KB)
    }

    // Last entry
    cmd_tbl->prdt_entry[i].dba = (uint32_t)((uint64_t)(size_t)buf);
    cmd_tbl->prdt_entry[i].dbau = (uint32_t)((uint64_t)((size_t)buf) >> 32);
    cmd_tbl->prdt_entry[i].dbc = (remaining_sectors << 9) - 1; // remaining sectors * 512 bytes - 1
    cmd_tbl->prdt_entry[i].i = 1;

    // Setup FIS as before
    fis_reg_h2d_t *fis = (fis_reg_h2d_t *)(&cmd_tbl->cfis);
    memset(fis, 0, sizeof(fis_reg_h2d_t));
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = 0x25;
    fis->device = 1 << 6;

    fis->lba0 = (uint8_t)(lba & 0xFF);
    fis->lba1 = (uint8_t)((lba >> 8) & 0xFF);
    fis->lba2 = (uint8_t)((lba >> 16) & 0xFF);
    fis->lba3 = (uint8_t)((lba >> 24) & 0xFF);
    fis->lba4 = (uint8_t)((lba >> 32) & 0xFF);
    fis->lba5 = (uint8_t)((lba >> 40) & 0xFF);

    fis->countl = sector_count & 0xFF;
    fis->counth = (sector_count >> 8) & 0xFF;

    port_base[0x10 / 4] = (uint32_t)-1; // Clear interrupts

    // Stop both engines
    port_base[0x18 / 4] &= ~0x11;
    while (port_base[0x18 / 4] & ((1 << 14) | (1 << 15))) { /* wait */ }

    // Clear PxIS etc., then enable FIS receive first
    port_base[0x18 / 4] |= (1 << 4);  // FRE

    // Now start command processing
    port_base[0x18 / 4] |= 1;  // ST

    phys_invalidate_cache(buffer, sector_count * 512);
    phys_flush_cache(port_mem, sizeof(ahci_port_mem_t));

    port_base[0x38 / 4] |= 1; // Issue command
    phys_invalidate_cache(buffer, 512);

    // Wait for command completion
    timeout = SATA_WAIT_TIMEOUT + sector_count * SATA_WAIT_TIMEOUT_PER_SECTOR; // Adjust timeout based on sector count. 10,000us + 2**16 * 200us = 10ms + 64,000 * 0.2ms = 10ms + 12.8s = 12.81s max for 65535 sectors
    bool error_detected = false;
    uint32_t error_status = 0;

    while ((port_base[0x38 / 4] & 1) && timeout--) {
        uint32_t isr = port_base[0x10 / 4];
        if (isr & ((1 << 30) | (1 << 31))) { // TFES or ERR
            error_detected = true;
            error_status = isr;
            break;
        }
        udelay(10);
    }

    if (error_detected) {
        if (error_status & (1 << 30)) {
            LOG_FATAL("READ DMA error: Data Transfer Error (TFES)\n");
            SERIAL(Info, sata_ahci_read_sector, "READ DMA error: Data Transfer Error (TFES)");
        }
        if (error_status & (1 << 31)) {             
            LOG_FATAL("READ DMA error: General Error (ERR)\n");
            SERIAL(Info, sata_ahci_read_sector, "READ DMA error: General Error (ERR)");
        }
        // Print other bits if needed
        free(port_mem);
        return 8;
    }

    if (timeout == 0) {
        LOG_FATAL("Timeout waiting for command completion\n");
        SERIAL(Info, sata_ahci_read_sector, "Timeout waiting for command completion");
        free(port_mem);
        return 9;
    }

    SERIAL(Info, sata_ahci_read_sector, "Sector read completed successfully");
    free(port_mem);
    return 0;
}



int sata_ahci_identify(volatile uint32_t *port_base, void *buffer) {
    SERIAL(Info, sata_ahci_identify, "Starting SATA device identification");
    ahci_port_mem_t *port_mem = malloc(sizeof(ahci_port_mem_t));
    port_base[0x00 / 4] = (uint32_t)((uint64_t)(size_t)port_mem->cl);
    port_base[0x04 / 4] = (uint32_t)(((uint64_t)(size_t)port_mem->cl) >> 32);
    port_base[0x08 / 4] = (uint32_t)((uint64_t)(size_t)port_mem->rfis);
    port_base[0x0C / 4] = (uint32_t)(((uint64_t)(size_t)port_mem->rfis) >> 32);

    // Stop engine and FIS receive engine
    port_base[0x18 / 4] &= ~0x01;
    port_base[0x18 / 4] &= ~(1 << 4);
    while (port_base[0x18 / 4] & (1 << 15));
    while (port_base[0x18 / 4] & (1 << 14));

    hba_cmd_header_t *cmd_header = (hba_cmd_header_t *)(port_mem->cl);
    cmd_header[0].flags = 5; // CFIS length 5 DWORDS, read=0
    cmd_header[0].prdt_length = 1;

    hba_cmd_tbl_t *cmd_tbl = (hba_cmd_tbl_t *)(port_mem->ct);
    cmd_header[0].ctba = (uint32_t)((uint64_t)(size_t)cmd_tbl);
    cmd_header[0].ctbau = (uint32_t)(((uint64_t)(size_t)cmd_tbl) >> 32);

    // PRDT points to buffer where IDENTIFY data will be stored
    cmd_tbl->prdt_entry[0].dba = (uint32_t)((uint64_t)(size_t)buffer);
    cmd_tbl->prdt_entry[0].dbau = (uint32_t)(((uint64_t)(size_t)buffer) >> 32);
    cmd_tbl->prdt_entry[0].dbc = 512 - 1;  // 512 bytes
    cmd_tbl->prdt_entry[0].i = 1;

    // Prepare command FIS for IDENTIFY DEVICE (ATA command 0xEC)
    fis_reg_h2d_t *fis = (fis_reg_h2d_t *)(&cmd_tbl->cfis);
    for (uint32_t i = 0; i < sizeof(fis_reg_h2d_t); i++) ((uint8_t *)fis)[i] = 0;
    fis->fis_type = 0x27;   // Register H2D
    fis->c = 1;             // Command
    fis->command = 0xEC;    // IDENTIFY DEVICE

    // Clear interrupts
    port_base[0x10 / 4] = (uint32_t)-1;

    // Start port engine
    port_base[0x18 / 4] |= 0x11;
    phys_invalidate_cache(buffer, 512);
    phys_flush_cache(port_mem, sizeof(ahci_port_mem_t));

    // Issue command
    port_base[0x38 / 4] |= 1;
    phys_invalidate_cache(buffer, 512);

    // Wait for completion
    uint32_t timeout = SATA_WAIT_TIMEOUT;
    while ((port_base[0x38 / 4] & 1) && timeout--) {
        if (port_base[0x10 / 4] & (1 << 30)) {
            LOG_FATAL("SATAPI READ error\n");
            SERIAL(Info, sata_ahci_identify, "SATAPI READ error during identification");
            free(port_mem);
            return 8;
        }
        udelay(10);
    }
    SERIAL(Info, sata_ahci_identify, "Device identification completed");
    free(port_mem);
    return 0;
}


int sata_ahci_identify_satapi(volatile uint32_t *port_base, void *buffer) {
    SERIAL(Info, sata_ahci_identify_satapi, "Starting SATAPI device identification");
    ahci_port_mem_t *port_mem = malloc(sizeof(ahci_port_mem_t));
    port_base[0x00 / 4] = (uint32_t)((uint64_t)(size_t)port_mem->cl);
    port_base[0x04 / 4] = (uint32_t)(((uint64_t)(size_t)port_mem->cl) >> 32);
    port_base[0x08 / 4] = (uint32_t)((uint64_t)(size_t)port_mem->rfis);
    port_base[0x0C / 4] = (uint32_t)(((uint64_t)(size_t)port_mem->rfis) >> 32);

    // Stop engine and FIS receive engine
    port_base[0x18 / 4] &= ~0x01;
    port_base[0x18 / 4] &= ~(1 << 4);
    while (port_base[0x18 / 4] & (1 << 15));
    while (port_base[0x18 / 4] & (1 << 14));

    hba_cmd_header_t *cmd_header = (hba_cmd_header_t *)(port_mem->cl);
    cmd_header[0].flags = 5; // CFIS length 5 DWORDS, read=0
    cmd_header[0].prdt_length = 1;

    hba_cmd_tbl_t *cmd_tbl = (hba_cmd_tbl_t *)(port_mem->ct);
    cmd_header[0].ctba = (uint32_t)((uint64_t)(size_t)cmd_tbl);
    cmd_header[0].ctbau = (uint32_t)(((uint64_t)(size_t)cmd_tbl) >> 32);

    // PRDT points to buffer where IDENTIFY PACKET DEVICE data will be stored
    cmd_tbl->prdt_entry[0].dba = (uint32_t)((uint64_t)(size_t)buffer);
    cmd_tbl->prdt_entry[0].dbau = (uint32_t)(((uint64_t)(size_t)buffer) >> 32);
    cmd_tbl->prdt_entry[0].dbc = 512 - 1;  // 512 bytes
    cmd_tbl->prdt_entry[0].i = 1;

    // Prepare command FIS for IDENTIFY PACKET DEVICE (ATAPI command 0xA1)
    fis_reg_h2d_t *fis = (fis_reg_h2d_t *)(&cmd_tbl->cfis);
    for (uint32_t i = 0; i < sizeof(fis_reg_h2d_t); i++) ((uint8_t*)fis)[i] = 0;
    fis->fis_type = 0x27;   // Register H2D
    fis->c = 1;             // Command
    fis->command = 0xA1;    // IDENTIFY PACKET DEVICE

    // Clear interrupts
    port_base[0x10 / 4] = (uint32_t)-1;

    // Start port engine
    port_base[0x18 / 4] |= 0x11;
    phys_invalidate_cache(buffer, 512);
    phys_flush_cache(port_mem, sizeof(ahci_port_mem_t));

    // Issue command
    port_base[0x38 / 4] |= 1;
    phys_invalidate_cache(buffer, 512);

    // Wait for completion
    
    uint32_t timeout = SATA_WAIT_TIMEOUT;
    while ((port_base[0x38 / 4] & 1) && timeout--) {
        if (port_base[0x10 / 4] & (1 << 30)) {
            LOG_FATAL("SATAPI READ error\n");
            SERIAL(Info, sata_ahci_identify_satapi, "SATAPI READ error during identification");
            free(port_mem);
            return 8;
        }
        udelay(10);
    }
    SERIAL(Info, sata_ahci_identify_satapi, "SATAPI identification completed");
    free(port_mem);
    return 0;
}

int sata_ahci_identify_satapi_properly(volatile uint32_t *port_base, void *buffer) {
    SERIAL(Info, sata_ahci_identify_satapi_properly, "Starting proper SATAPI identification");
    if (!buffer) return -1;

    ahci_port_mem_t *port_mem = malloc(sizeof(ahci_port_mem_t));
    if (!port_mem) return -1;
    memset(port_mem, 0, sizeof(ahci_port_mem_t));

    uint64_t cl_phys   = (uint64_t)(size_t)virt_to_phys(port_mem->cl);
    uint64_t rfis_phys = (uint64_t)(size_t)virt_to_phys(port_mem->rfis);
    uint64_t ct_phys   = (uint64_t)(size_t)virt_to_phys(port_mem->ct);
    uint64_t buf_phys  = (uint64_t)(size_t)virt_to_phys(buffer);

    // Setup PxCLB and PxFB
    port_base[0x00 / 4] = (uint32_t)(cl_phys & 0xFFFFFFFF);
    port_base[0x04 / 4] = (uint32_t)(cl_phys >> 32);
    port_base[0x08 / 4] = (uint32_t)(rfis_phys & 0xFFFFFFFF);
    port_base[0x0C / 4] = (uint32_t)(rfis_phys >> 32);

    // Stop command engine
    port_base[0x18 / 4] &= ~(1 | (1 << 4));
    uint32_t timeout = SATA_WAIT_TIMEOUT;
    while ((port_base[0x18 / 4] & ((1 << 14) | (1 << 15))) && timeout--)
        udelay(10);
    if (timeout == 0)
        goto fail;

    // Build command header
    hba_cmd_header_t *cmd_header = (hba_cmd_header_t *)port_mem->cl;
    memset(cmd_header, 0, sizeof(hba_cmd_header_t));
    cmd_header[0].prdt_length = 1;
    cmd_header[0].flags = (5) | (1 << 5); // CFL=5, ATAPI=1, Read=0

    // Command table
    hba_cmd_tbl_t *cmd_tbl = (hba_cmd_tbl_t *)port_mem->ct;
    cmd_header[0].ctba  = (uint32_t)(ct_phys & 0xFFFFFFFF);
    cmd_header[0].ctbau = (uint32_t)(ct_phys >> 32);

    cmd_tbl->prdt_entry[0].dba  = (uint32_t)(buf_phys & 0xFFFFFFFF);
    cmd_tbl->prdt_entry[0].dbau = (uint32_t)(buf_phys >> 32);
    cmd_tbl->prdt_entry[0].dbc  = (8 - 1) & 0x003FFFFF; // 8 bytes expected
    cmd_tbl->prdt_entry[0].i    = 1;

    // Build ATAPI 12-byte READ CAPACITY(10)
    uint8_t atapi_cmd[12];
    build_read_capacity_atapi_cmd(atapi_cmd);
    memset(cmd_tbl->acmd, 0, 16);
    memcpy(cmd_tbl->acmd, atapi_cmd, 12);

    // H2D FIS
    fis_reg_h2d_t *fis = (fis_reg_h2d_t *)(&cmd_tbl->cfis);
    memset(fis, 0, sizeof(fis_reg_h2d_t));
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = 0xA0; // PACKET command
    fis->device = 0xA0;

    // Clear interrupts
    port_base[0x10 / 4] = 0xFFFFFFFF;
    port_base[0x30 / 4] = 0xFFFFFFFF;

    // Start engines
    port_base[0x18 / 4] |= (1 << 4);
    port_base[0x18 / 4] |= 1;

    // Issue
    port_base[0x38 / 4] = 1;

    // Wait
    timeout = SATA_WAIT_TIMEOUT;
    while ((port_base[0x38 / 4] & 1) && timeout--) udelay(10);
    if (timeout == 0) goto fail;

    read10_capabillity_buffer_t *rbuf = buffer;
    rbuf->sector_size = be32toh(rbuf->sector_size);
    rbuf->last_lba = be32toh(rbuf->last_lba);

    SERIAL(Info, sata_ahci_identify_satapi_properly, "SATAPI identification successful");
    free(port_mem);
    return 0;

fail:
    SERIAL(Info, sata_ahci_identify_satapi_properly, "SATAPI identification failed");
    free(port_mem);
    return -1;
}





/* helper: dump RFIS error/status if available 
static void ahci_dump_rfis(ahci_port_mem_t *port_mem) {
    uint8_t *rfis = (uint8_t *)port_mem->rfis;
    // Register – Device to Host FIS starts at offset 0x00 in RFIS area
    terminal_printf("RFIS: type=0x%x, status=0x%x, error=0x%x\n",
        rfis[0],        // FIS type (should be 0x34)
        rfis[2],        // Status
        rfis[3]);       // Error
}*/

int sata_ahci_read_sector_satapi(volatile uint32_t *port_base, uint32_t lba, uint16_t sector_count, void *buffer) {
    SERIAL(Info, sata_ahci_read_sector_satapi, "Starting SATAPI sector read");
    if (!buffer || sector_count == 0) {
        LOG_FATAL("Null buffer or zero sector count\n");
        SERIAL(Info, sata_ahci_read_sector_satapi, "Null buffer or zero sector count");
        return -1;
    }

    ahci_port_mem_t *port_mem = malloc(sizeof(ahci_port_mem_t));
    if (!port_mem) {
        LOG_FATAL("Failed to allocate port memory\n");
        SERIAL(Info, sata_ahci_read_sector_satapi, "Failed to allocate port memory");
        return -1;
    }
    memset(port_mem, 0, sizeof(ahci_port_mem_t));

    uint64_t cl_phys   = (uint64_t)(size_t)virt_to_phys(port_mem->cl);
    uint64_t rfis_phys = (uint64_t)(size_t)virt_to_phys(port_mem->rfis);
    uint64_t ct_phys   = (uint64_t)(size_t)virt_to_phys(port_mem->ct);
    uint64_t buf_phys  = (uint64_t)(size_t)virt_to_phys(buffer);

    
    // Setup PxCLB and PxFB (command list and received FIS base)
    port_base[0x00 / 4] = (uint32_t)(cl_phys & 0xFFFFFFFF);
    port_base[0x04 / 4] = (uint32_t)(cl_phys >> 32);
    port_base[0x08 / 4] = (uint32_t)(rfis_phys & 0xFFFFFFFF);
    port_base[0x0C / 4] = (uint32_t)(rfis_phys >> 32);

    // Stop command engine if running
    port_base[0x18 / 4] &= ~(1 | (1 << 4));  // clear ST and FRE
    uint32_t timeout = SATA_WAIT_TIMEOUT;
    while ((port_base[0x18 / 4] & ((1 << 14) | (1 << 15))) && timeout--)
        udelay(10);
    if (timeout == 0)
        goto fail;

    // Build command header
    hba_cmd_header_t *cmd_header = (hba_cmd_header_t *)port_mem->cl;
    memset(cmd_header, 0, sizeof(hba_cmd_header_t));
    cmd_header[0].prdt_length = 1;
    cmd_header[0].flags = (5) | (1 << 5); // CFL=5, ATAPI=1, Read=0 (read op)

    // Build command table
    hba_cmd_tbl_t *cmd_tbl = (hba_cmd_tbl_t *)port_mem->ct;
    cmd_header[0].ctba  = (uint32_t)(ct_phys & 0xFFFFFFFF);
    cmd_header[0].ctbau = (uint32_t)(ct_phys >> 32);

    uint32_t byte_count = sector_count * 2048;
    cmd_tbl->prdt_entry[0].dba  = (uint32_t)(buf_phys & 0xFFFFFFFF);
    cmd_tbl->prdt_entry[0].dbau = (uint32_t)(buf_phys >> 32);
    cmd_tbl->prdt_entry[0].dbc  = (byte_count - 1) & 0x003FFFFF;
    cmd_tbl->prdt_entry[0].i    = 1;

    // Build ATAPI 12-byte READ(10)
    uint8_t atapi_cmd[12];
    build_read10_atapi_cmd(atapi_cmd, lba, sector_count);
    memset(cmd_tbl->acmd, 0, 16);
    memcpy(cmd_tbl->acmd, atapi_cmd, 12);

    // Build H2D Register FIS
    fis_reg_h2d_t *fis = (fis_reg_h2d_t *)(&cmd_tbl->cfis);
    memset(fis, 0, sizeof(fis_reg_h2d_t));
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = 0xA0; // PACKET command
    fis->device = 0xA0;  // ATAPI device
    fis->featurel = 0;
    fis->featureh = 0;
    fis->countl = 0;
    fis->counth = 0;

    // Clear status registers
    port_base[0x10 / 4] = 0xFFFFFFFF; // PxIS
    port_base[0x30 / 4] = 0xFFFFFFFF; // PxSERR

    // Start FIS receive engine
    port_base[0x18 / 4] |= (1 << 4); // FRE
    port_base[0x18 / 4] |= 1;        // ST

    // Issue command (slot 0)
    port_base[0x38 / 4] = 1;

    // Wait for completion
    timeout = SATA_WAIT_TIMEOUT + sector_count * SATA_WAIT_TIMEOUT_PER_SECTOR;
    while ((port_base[0x38 / 4] & 1) && timeout--) {
        uint32_t isr = port_base[0x10 / 4];
        if (isr & ((1 << 30) | (1 << 31))) { // Task file or host bus error
            LOG_FATAL("AHCI Error at ISR=0x%x\n", isr);
            SERIAL(Info, sata_ahci_read_sector_satapi, "AHCI Error during read");
            goto fail;
        }
        udelay(10);
    }

    if (timeout == 0)
        goto fail;

    timeout = SATA_WAIT_TIMEOUT;
    while (port_base[0x38 / 4] & 1 && timeout--) udelay(10); // wait slot clear
    SERIAL(Info, sata_ahci_read_sector_satapi, "SATAPI sector read completed");
    free(port_mem);
        return 0;

fail:
    
    LOG_FATAL("Timeout or failure in AHCI-ATAPI read.\n");
    SERIAL(Info, sata_ahci_read_sector_satapi, "Timeout or failure in AHCI-ATAPI read");

    udelay(50);  // Wait for some ports
    uint32_t isr = port_base[0x10 / 4];
       printf("PxIS=0x%x PxTFD=0x%x PxSERR=0x%x\n",
                    isr, port_base[0x20 / 4], port_base[0x30 / 4]);
    free(port_mem);
    return -1;
}


storage_device_t sata_temp_devices[128]; 
uint32_t sata_temp_count = 0;

// Dummy AHCI MMIO base (would normally be discovered by PCI scanning)
static uint32_t dummy_mmio_base = 0xF0000000;

void sata_init(void) {
    LOG_INFO("Detecting drives...\n");
    SERIAL(Info, sata_init, "Detecting drives...\n");
    storage_device_t dev = {0};
    dev.type_identifier = 'S';
    dev.sector_size = 512;
    dev.device_data = NULL; // no real identify data yet

    sata_temp_devices[0] = dev;
    sata_temp_count = 1;

    LOG_INFO("Detected %u device\n", sata_temp_count);
    SERIAL(Info, sata_init, "Detected %u device\n", sata_temp_count);
    // If you want, call sata_search() here and actually fill sata_temp_devices
    // sata_search(dummy_mmio_base);
}