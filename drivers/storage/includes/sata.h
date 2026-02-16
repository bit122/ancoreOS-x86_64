/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: sata.h
    Description: SATA module for the VNiX Operating System.
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

#ifndef DRIVERS_SATA_H
#define DRIVERS_SATA_H

#include <stdint.h>
#define SATA_WAIT_TIMEOUT 1000
#define SATA_WAIT_TIMEOUT_PER_SECTOR 250

typedef struct {
    uint8_t cl[1024];   // Command List
    uint8_t rfis[256];  // Received FIS
    uint8_t ct[256 * 32]; // Command Tables, 32 entries, each 256 bytes
} ahci_port_mem_t;

typedef struct {
    uint8_t fis_type;
    uint8_t pmport : 4;
    uint8_t rsv0 : 3;
    uint8_t c : 1;
    uint8_t command;
    uint8_t featurel;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;

    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;

    uint8_t rsv1[4];
} fis_reg_h2d_t;

typedef struct {
    uint16_t flags;
    uint16_t prdt_length;
    uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsv1[4];
} hba_cmd_header_t;

typedef struct {
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
    uint32_t dbc : 22;
    uint32_t rsv1 : 9;
    uint32_t i : 1;
} hba_prdt_entry_t;

typedef struct {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t rsv[48];
    hba_prdt_entry_t prdt_entry[1];
} hba_cmd_tbl_t;

int sata_ahci_read_sector(volatile uint32_t *port_base, uint64_t lba, uint16_t sector_count, void *buffer);
int sata_ahci_identify(volatile uint32_t *port_base, void *buffer);
void sata_print_sector_size(uint16_t *identify_data);

int sata_ahci_read_sector_satapi(volatile uint32_t *port_base, uint32_t lba, uint16_t sector_count, void *buffer);
int sata_ahci_identify_satapi(volatile uint32_t *port_base, void *buffer);
// Needs an 8 byte buffer. Buffer will contain uint32_t size_lba (Big Endian) and size_sector (Big Endian)
int sata_ahci_identify_satapi_properly(volatile uint32_t *port_base, void *buffer);
void sata_search(uint32_t mmio_base);
void sata_init(void);
#endif // DRIVERS_SATA_H