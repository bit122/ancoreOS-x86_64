/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: stinit.c
    Description: SATA initialization module for the VNiX Operating System
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

#include "includes/stinit.h"
#include "includes/ata.h"
#include "includes/atapi.h"
#include "includes/sata.h"
#include "tools/includes/log-info.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
//storage devices array (global)
storage_device_t storage_devices[MAX_STORAGE_DEVICES];
uint32_t storage_device_count = 0;

// add device to the array
static void register_device(storage_device_t dev) {
    if (storage_device_count >= MAX_STORAGE_DEVICES) {
        LOG_WARN("Max storage devices reached!\n");
        
        return;
    }
    storage_devices[storage_device_count++] = dev;
    sata_init();
    LOG_INFO("");
    printf("Registered device type '%c', sector_size=%u\n", dev.type_identifier, (uint32_t)dev.sector_size);

    SERIAL(Info, register_device, "Registered device type '%c', sector_size=%u\n", dev.type_identifier, (uint32_t)dev.sector_size);

}

void storage_init(void) {
    //ata_init();
    for (uint8_t drive = 0; drive < 2; drive++) { // primary master/slave
        uint16_t identify_data[256];
        if (identify_ata(drive, identify_data) == 0) {
            storage_device_t dev = {0};
            dev.type_identifier = 'P';
            dev.sector_size = 512;
            dev.device_data = malloc(sizeof(uint16_t) * 256);
            memcpy(dev.device_data, identify_data, sizeof(uint16_t) * 256);
            register_device(dev);
        }
    }

    //atapi_init();
    for (uint8_t drive = 0; drive < 2; drive++) {
        read10_capabillity_buffer_t buffer;
        if (identify_atapi_useful(&buffer, drive) == 0) {
            storage_device_t dev = {0};
            dev.type_identifier = 'Q';
            dev.sector_size = buffer.sector_size;
            dev.device_data = malloc(sizeof(buffer));
            memcpy(dev.device_data, &buffer, sizeof(buffer));
            register_device(dev);
        }
    }

     // internally calls sata_search(mmio_base)


    // sata_search already logs detected drives; here, i register them
    // for simplicity, i will assume sata_search fills a temp array we can loop over:
    extern storage_device_t sata_temp_devices[];
    extern uint32_t sata_temp_count;
    for (uint32_t i = 0; i < sata_temp_count; i++) {
        register_device(sata_temp_devices[i]);
    }

    LOG_INFO("Storage initialization complete, %u device(s) found.\n",
        storage_device_count);
    SERIAL(Info, storage_init, "Storage initialization complete, %u device(s) found.\n",
        storage_device_count);
}
