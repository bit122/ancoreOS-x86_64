
/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: atapi.h
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

#ifndef DRIVERS_ATAPI_H
#define DRIVERS_ATAPI_H

#include <stdint.h>
#include <stdbool.h>
#include "scsi.h"

#define ATAPI_CMD_PACKET    0xA0
#define ATAPI_CMD_READ10    0x28
#define ATAPI_CMD_IDENTIFY  0xA1

#define ATA_REG_FEATURES    1  // Same as ERROR register, just written to


#ifdef __cplusplus
extern "C" {
#endif

static const char *const atapi_errors[] = {
    "", "", "", "",
    "", "", "", "",
    "", "", "", "",
    "", "NO DATA", "DISK NOT READY", "DISK BUSY",
};

// ATA command status and error reporting. static and should be changed soon
extern uint8_t atapi_status;
extern uint8_t atapi_error;

int atapi_read_sector(uint32_t lba, uint16_t sector_count, void *buffer, uint8_t drive, uint32_t sector_size);
int atapi_read_sector_manual(uint32_t lba, uint16_t sector_count, void *buffer, uint8_t drive, uint16_t pmio_base, uint32_t sector_size);
int identify_atapi(uint8_t drive, uint16_t identify_data[256]);
int identify_atapi_manual(uint8_t drive, uint16_t identify_data[256], uint16_t pmio_base);
int identify_atapi_useful(read10_capabillity_buffer_t *buffer, uint8_t drive);
int identify_atapi_useful_manual(read10_capabillity_buffer_t *buffer, uint8_t drive, uint16_t pmio_base);

#ifdef __cplusplus
}
#endif

#endif // ATAPI_H
