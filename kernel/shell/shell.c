/*
    Copyright (C) 2026 Aspen Software Foundation

    Module: shell.c
    Description: Shell module for the VNiX Operating System.
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

#define _KERNEL //for PAGE_SIZE in <limits.h>
#include <string.h>  
#include "tools/includes/serial.h"
#include "includes/keyboard.h"
#include "tools/includes/log-info.h"
#include "kernel/terminal/src/flanterm.h"
#include <stdio.h>
#include "mm/includes/pmm.h"
#include "arch/x86_64/includes/isr.h"
#include <stdlib.h>
#include <limits.h>

extern struct flanterm_context *global_flanterm;

typedef enum {
    SHCMD_HELP,
    SHCMD_ECHO,
    SHCMD_CLEAR,
    SHCMD_PMMSTATS,
    SHCMD_PANIC,
    SHCMD_BIRDSAY,
    SHCMD_UPTIME,
    SHCMD_UNKNOWN 
} shell_command_t;

shell_command_t get_command(const char *buffer) {
    if (strcmp(buffer, "help") == 0) return SHCMD_HELP;
    if (strcmp(buffer, "echo") == 0) return SHCMD_ECHO;
    if (strcmp(buffer, "clear") == 0) return SHCMD_CLEAR;
    if (strcmp(buffer, "pmmstats") == 0) return SHCMD_PMMSTATS;
    if (strcmp(buffer, "panic") == 0) return SHCMD_PANIC;
    if (strcmp(buffer, "birdsay") == 0) return SHCMD_BIRDSAY;
    if (strcmp(buffer, "uptime") == 0) return SHCMD_UPTIME;

    return SHCMD_UNKNOWN;
}

void print_string(const char* str) {
    printf("%s", str);
}

void print_char(char c) {
    printf("%c", c);
}

void cmd_help(void) {
    printf("Available commands:\n");
    printf("  help      - Show this help\n");
    printf("  echo      - Echo arguments\n");
    printf("  clear     - Clear screen\n");
    printf("  pmmstats  - Gets the PMM stats\n");
    printf("  panic     - Calls a kernel panic\n");
    printf("  birdsay   - Cowsay, but with a bird\n");
    printf("  uptime    - Gets the uptime in seconds\n");
}

void cmd_clear(void) {
    if (global_flanterm) {
        flanterm_write(global_flanterm, "\033[2J", 4);  // Clear screen
        flanterm_write(global_flanterm, "\033[H", 3);   // Move cursor to home
    }
}

void uptime(void){

    printf("Uptime: %d second(s)\n", get_time_ms()/1000);

}

void pmmstats(void) {
    uint64_t total = pmm_get_total_pages();
    uint64_t free  = pmm_get_free_pages();
    uint64_t used  = pmm_get_used_pages();
    
    printf("PMM Statistics:\n");
    printf("  Total pages: %llu pages (%llu MB)\n", 
           total, (total * PAGE_SIZE) / (1024 * 1024));
    printf("  Free pages : %llu pages (%llu MB)\n", 
           free, (free * PAGE_SIZE) / (1024 * 1024));
    printf("  Used pages : %llu pages (%llu MB)\n", 
           used, (used * PAGE_SIZE) / (1024 * 1024));
    printf("  Page size  : %llu KB\n", PAGE_SIZE / 1024);
}

void spanic(void) {
    panic(NULL);
}

void execute(char* buffer) {
    // skip leading spaces
    while (*buffer == ' ') buffer++;
    
    // if empty, just return
    if (*buffer == '\0') return;
    
    // find cmd end
    char* args = buffer;
    while (*args && *args != ' ') args++;
    
    // seperate cmd from args
    int has_args = 0;
    if (*args) {
        *args = '\0';
        args++;
        while (*args == ' ') args++;
        has_args = (*args != '\0');
    }

    switch (get_command(buffer)) {
        case SHCMD_HELP:
            cmd_help();
            break;

        case SHCMD_ECHO:
            if (has_args) {
                print_string(args);
                printf("\n");
            } else {
                print_char('\n');
            }
            break;

        case SHCMD_CLEAR:
            cmd_clear();
            break;

        case SHCMD_PMMSTATS:
            pmmstats();
            break;

        case SHCMD_PANIC:
            spanic();
            break;
        case SHCMD_UPTIME:
            uptime();
            break;
        case SHCMD_BIRDSAY:
            if (has_args) {
                printf("   \\\\\n");
                printf("   (o>\n");
                printf("\\\\_//)\n");
                printf(" \\_/_)\n");
                printf("  _|_  ----> ");
                print_string(args);
                printf("\n");
            } else {
                print_char('\n');
            }
            break;

        default:
            printf("Unknown command: %s", buffer);
            //print_string(buffer);
            printf("\nType 'help' for available commands.\n");
            break;
    }
}

// Main shell loop
void shell_main(void) {
    char buffer[256];
    int pos;
    char c;
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║   VNiX Interactive Shell v1.2          ║\n");
    printf("║   Type 'help' for available commands   ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    while (1) {
        printf("vnix@root:$> ");
        
        pos = 0;
        while (1) {
            c = getc();
            
            if (c == '\n' || c == '\r') {
                printf("\n");
                break;
            } else if (c == '\b' || c == 127) {  // Backspace or DEL
                if (pos > 0) {
                    pos--;
                    printf("\b \b");
                }
            } else if (c >= 32 && c < 127 && pos < 255) { //see if its a printable char
                buffer[pos++] = c;
                printf("%c", c);  // echo char
            }
        }
        
        buffer[pos] = '\0';
        
        execute(buffer);
    }
}