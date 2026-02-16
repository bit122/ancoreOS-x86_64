/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: storage.c
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

#include "includes/storage.h"
#include "includes/ata.h"
#include "includes/atapi.h"
#include "includes/sata.h"
#include "includes/scsi.h"
#include "kernel/time/includes/time.h"


int drive_read(storage_device_t *device, uint64_t lba, uint32_t sector_count, void *buf) {
    int res;
    switch (device->type_identifier) {
        case 'P': // ATA auto
            res = ata_read_sector_auto(
                lba, (uint16_t)sector_count, buf, device->device_data.pata.drive_number, device->sector_size);
                        return res;

        case 'Q': // ATAPI auto
            res = atapi_read_sector(
                (uint32_t)lba, (uint16_t)sector_count, buf, device->device_data.pata.drive_number, device->sector_size);
                        return res;

        case 'p': { // ATA manual
            uint16_t base_port = device->device_data.pata.pmio_base;
            res = ata_read_sector_semiauto(
                lba, (uint16_t)sector_count, buf, base_port, device->device_data.pata.drive_number, device->sector_size);
                        return res;
        }

        case 'q': { // ATAPI manual (Really old and small drive without 48 bit sector addressing)
            uint16_t base_port = device->device_data.pata.pmio_base;
            res = atapi_read_sector_manual(
                (uint32_t)lba, (uint16_t)sector_count, buf, device->device_data.pata.drive_number, base_port, device->sector_size);
                        return res;
        }

        case 'S': // SATA AHCI ATA
            res = sata_ahci_read_sector(
                (volatile uint32_t *)(device->device_data.sata.port_base), lba, 
                (uint16_t)sector_count, buf);
                        return res;
            

        case 'T': // SATA AHCI ATAPI
            res = sata_ahci_read_sector_satapi(
                (volatile uint32_t *)(device->device_data.sata.port_base), (uint32_t)lba, 
                (uint16_t)sector_count, buf);
                        return res;

        default:
            return 9; // unknown drive type
    }
}