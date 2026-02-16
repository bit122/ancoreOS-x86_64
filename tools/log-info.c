/*
    Copyright (C) 2026 Aspen Software Foundation
    Module: log-info.c
    Description: The log module for the VNiX Operating System.
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
#include "includes/log-info.h"
#include <stdarg.h>
#include "kernel/time/includes/time.h"

extern struct flanterm_context *global_flanterm;

const char* result_str[ResultCount] = {
    [Ok]    = "OK: ",
    [Info]  = "INFO: ",
    [Warn]  = "WARN: ",
    [Fatal] = "FATAL: ",
};

static inline char* strcpy_advance(char *dest, const char *src) {
    while (*src) *dest++ = *src++;
    return dest;
}

void log_to_terminal(result_t status, const char *from, const char *file, int line, const char *fmt, ...) {
    __attribute__((format(printf, 5, 6))); // <<<vvv this too!
    if (!global_flanterm) return;
    if (!from) from = "<unknown>";  //added these so the kernel doesnt eat shit if this fails
    if (!file) file = "<unknown>";
    if (!fmt) return;  

    char message[1024];
    char *ptr = message;
    
    // get timestamp
    uint64_t ms = get_time_ms();
    uint32_t secs = ms / 1000;
    uint32_t msecs = ms % 1000;
    
    *ptr++ = '[';
    if (secs < 10) { *ptr++ = ' '; *ptr++ = ' '; }
    else if (secs < 100) { *ptr++ = ' '; }
    
    char sec_str[16];
    itoa(secs, sec_str, 10);
    ptr = strcpy_advance(ptr, sec_str);
    
    *ptr++ = '.';
    if (msecs < 100) *ptr++ = '0';
    if (msecs < 10) *ptr++ = '0';
    
    char ms_str[16];
    itoa(msecs, ms_str, 10);
    ptr = strcpy_advance(ptr, ms_str);
    
    *ptr++ = ']';
    *ptr++ = ' ';
    
    const char *color = get_status_color(status);
    ptr = strcpy_advance(ptr, color);
    
    // add status prefix (WARN: or FATAL:)
    const char *status_str = result_str[status];
    ptr = strcpy_advance(ptr, status_str);
    
    // reset color after status prefix if it was colored
    if (status != Info) {
        ptr = strcpy_advance(ptr, COLOR_RESET);
    }
    
    ptr = strcpy_advance(ptr, COLOR_DIM);
    *ptr++ = '[';
    ptr = strcpy_advance(ptr, from);
    ptr = strcpy_advance(ptr, " in ");
    
    // extract just the filename from the full path
    const char *filename = file;
    const char *last_slash = file;
    while (*file) {
        if (*file == '/' || *file == '\\') {
            last_slash = file + 1;
        }
        file++;
    }
    ptr = strcpy_advance(ptr, last_slash);
    
    *ptr++ = ':';
    
    // add line number
    char line_str[16];
    itoa(line, line_str, 10);
    ptr = strcpy_advance(ptr, line_str);
    
    *ptr++ = ']';
    ptr = strcpy_advance(ptr, COLOR_RESET);
    *ptr++ = ' ';
    
    // process the format string and arguments
    va_list args;
    va_start(args, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 'd' || *fmt == 'i') {
                int val = va_arg(args, int);
                char num[16];
                itoa(val, num, 10);
                ptr = strcpy_advance(ptr, num);
            } else if (*fmt == 'u') {
                unsigned int val = va_arg(args, unsigned int);
                char num[16];
                itoa((int)val, num, 10);
                ptr = strcpy_advance(ptr, num);
            } else if (*fmt == 's') {
                char *str = va_arg(args, char*);
                if (str) {
                    ptr = strcpy_advance(ptr, str);
                }
            } else if (*fmt == 'p' || *fmt == 'x') {
                unsigned long val = va_arg(args, unsigned long);
                *ptr++ = '0';
                *ptr++ = 'x';
                char hex[20];
                itoa((int)val, hex, 16);
                ptr = strcpy_advance(ptr, hex);
            } else if (*fmt == '%') {
                *ptr++ = '%';
            } else {
                // unknown format specifier, just copy it
                *ptr++ = '%';
                *ptr++ = *fmt;
            }
        } else {
            *ptr++ = *fmt;
        }
        if (*fmt) fmt++;
    }
    
    va_end(args);
    *ptr = '\0';
    
    flanterm_write(global_flanterm, message, ptr - message);
}

void printcol(const char *color, const char *text) {
    if (global_flanterm) {
        flanterm_write(global_flanterm, color, strlen(color));
        flanterm_write(global_flanterm, text, strlen(text));
        flanterm_write(global_flanterm, COLOR_RESET, strlen(COLOR_RESET));
    }
}