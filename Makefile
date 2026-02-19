# Copyright (C) 2026 Aspen Software Foundation

# 	Module: Makefile	
# 	Description: The Makefile for the VNiX Operating System.
# 	Author: Yazin Tantawi (and Jerry Jhird)

# 	All components of the VNiX Operating System, except where otherwise noted,
# 	are copyright of the Aspen Software Foundation (and the corresponding author(s)) and licensed under GPLv2 or later.
# 	For more information on the GNU Public License Version 2, please refer to the LICENSE file 
# 	or to the link provided here: https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html

# THIS OPERATING SYSTEM IS PROVIDED "AS IS" AND "AS AVAILABLE" UNDER
# THE GNU GENERAL PUBLIC LICENSE VERSION 2, WITHOUT
# WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
# TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE, TITLE, AND NON-INFRINGEMENT.

# TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, IN NO EVENT SHALL
# THE AUTHORS, COPYRIGHT HOLDERS, OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE), ARISING IN ANY WAY OUT OF THE USE OF THIS OPERATING SYSTEM,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE OPERATING SYSTEM IS
# WITH YOU. SHOULD THE OPERATING SYSTEM PROVE DEFECTIVE, YOU ASSUME THE COST OF
# ALL NECESSARY SERVICING, REPAIR, OR CORRECTION.

# YOU SHOULD HAVE RECEIVED A COPY OF THE GNU GENERAL PUBLIC LICENSE
# ALONG WITH THIS OPERATING SYSTEM; IF NOT, WRITE TO THE FREE SOFTWARE
# FOUNDATION, INC., 51 FRANKLIN STREET, FIFTH FLOOR, BOSTON,
# MA 02110-1301, USA.

RED    := \033[0;31m
GREEN  := \033[0;32m
YELLOW := \033[1;33m
BLUE   := \033[0;34m
MAGENTA := \033[0;35m
CYAN   := \033[0;36m
NC     := \033[0m  # No Color

CFLAGS := \
	-I . \
	-I klibc/includes \
	-nostdinc \
	-ffreestanding \
	-Wall -Wextra -Wunused-parameter -Wimplicit-function-declaration \
	-fno-pie -no-pie \
	-mno-red-zone \
	-mcmodel=large \
	-D_ALLOC_SKIP_DEFINE

LDFLAGS := \
	-nostdlib \
	-static \
	-z noexecstack \
	-T linker.ld
QEMU_CPU ?=
QEMU_MEM ?= -m 1G

all: clean build/kernel.elf build/uefi-usb.img 

# Yes, yes, i know this is horrendous, but i dont care enough to change it, since it works.

build/kernel.elf:
	mkdir -p build
	@echo "$(CYAN)=-=-=-=-=-=-=-=-=-=-=-=-=-=-$(NC)"
	@echo "$(YELLOW)Compiling kernel and drivers...$(NC)"
	gcc -c kernel/kernel.c -o build/kernel.o $(CFLAGS)
	gcc -c kernel/terminal/src/flanterm.c -o build/flanterm.o $(CFLAGS)
	gcc -c kernel/terminal/src/flanterm_backends/fb.c -o build/fb.o $(CFLAGS)
	gcc -c klibc/string.c -o build/string.o $(CFLAGS)
	gcc -c klibc/limits.c -o build/limits.o $(CFLAGS)
	gcc -c klibc/stdio.c -o build/stdio.o $(CFLAGS)
	gcc -c klibc/stdlib.c -o build/stdlib.o $(CFLAGS)
	gcc -c mm/heapalloc/tlsf.c -o build/tlsf.o $(CFLAGS) 
	gcc -c tools/log-info.c -o build/log-info.o $(CFLAGS) 
	gcc -c tools/serial.c -o build/serial.o $(CFLAGS) 
	gcc -c arch/x86_64/gdt.c -o build/gdt.o $(CFLAGS)
	gcc -c arch/x86_64/idt.c -o build/idt.o $(CFLAGS)
	gcc -c arch/x86_64/io.c -o build/io.o $(CFLAGS)
	gcc -c mm/pmm.c -o build/pmm.o $(CFLAGS)
	gcc -c mm/vmm.c -o build/vmm.o $(CFLAGS)
	gcc -c arch/x86_64/isr.c -o build/isr.o $(CFLAGS)
	gcc -c arch/x86_64/isrs_gen.c -o build/isrs_gen.o $(CFLAGS)
	gcc -c drivers/pic/pic.c -o build/pic.o $(CFLAGS)
	gcc -c drivers/pic/pic_irq.c -o build/pic_irq.o $(CFLAGS)
	gcc -c kernel/shell/keyboard.c -o build/keyboard.o $(CFLAGS)
	gcc -c  drivers/storage/storage.c -o build/storage.o $(CFLAGS)
	gcc -c  drivers/storage/sata.c -o build/sata.o $(CFLAGS)
	gcc -c drivers/pic/apic/apic.c -o build/apic.o $(CFLAGS)
	gcc -c drivers/pic/apic/apic_irq.c -o build/apic_irq.o $(CFLAGS)
	gcc -c  drivers/storage/ata.c -o build/ata.o $(CFLAGS)
	gcc -c  drivers/storage/stinit.c -o build/stinit.o $(CFLAGS)
	gcc -c  drivers/storage/atapi.c -o build/atapi.o $(CFLAGS)
	gcc -c kernel/time/time.c -o build/time.o $(CFLAGS)
	gcc -c kernel/shell/shell.c -o build/shell.o $(CFLAGS)
	gcc -c tools/pit.c -o build/pit.o $(CFLAGS)
	gcc -c kernel/time/tsc.c -o build/tsc.o $(CFLAGS)
	gcc -c drivers/hci/ehci.c -o build/ehci.o $(CFLAGS)
	gcc -c drivers/pci/pci.c -o build/pci.o $(CFLAGS)
	nasm -f elf64 arch/x86_64/includes/asm/isr_stubs.asm -o build/isr_stubs.o
	
# After much research, i've concluded on this linking order because it looks much better than the hellish alternative i initially had
	@echo "$(CYAN)=-=-=-=-=-=-=-=-=-=-=-=-=-=-$(NC)"
	@echo "$(GREEN)Linking kernel...$(NC)"
	ld $(LDFLAGS) -o build/kernel.elf \
		build/kernel.o \
		build/flanterm.o \
		build/fb.o \
		build/string.o\
		build/stdio.o \
		build/tlsf.o \
		build/stdlib.o \
		build/log-info.o \
		build/gdt.o \
		build/isr.o\
		build/idt.o \
		build/pmm.o \
		build/vmm.o \
		build/io.o\
		build/isrs_gen.o\
		build/isr_stubs.o\
		build/serial.o \
		build/apic_irq.o\
		build/apic.o\
		build/storage.o\
		build/keyboard.o\
		build/pic_irq.o\
		build/pic.o\
		build/ata.o\
		build/atapi.o\
		build/stinit.o\
		build/sata.o\
		build/time.o\
		build/shell.o\
		build/tsc.o\
		build/pit.o\
		build/pci.o\
		build/ehci.o\
		build/limits.o 

	@echo "$(MAGENTA)Stripping debug info...$(NC)"
	objcopy --strip-debug build/kernel.elf


build/uefi-usb.img: kernel
	@echo "$(CYAN)=-=-=-=-=-=-=-=-=-=-=-=-=-=-$(NC)"
	@echo "$(BLUE)Creating raw UEFI image...$(NC)"
	
	rm -rf build/usb_root
	mkdir -p build/usb_root/EFI/BOOT build/usb_root/boot
	
	cp boot/BOOTX64.EFI build/usb_root/EFI/BOOT/
	cp build/kernel.elf build/usb_root/boot/
	cp boot/Assets/wallpaper4k.png build/usb_root/boot/ 2>/dev/null || true
	cp boot/Assets/limine.conf build/usb_root/
	
	echo "fs0:" > build/usb_root/startup.nsh
	echo "cd \\EFI\\BOOT" >> build/usb_root/startup.nsh
	echo "BOOTX64.EFI" >> build/usb_root/startup.nsh
	@echo "$(CYAN)=-=-=-=-=-=-=-=-=-=-=-=-=-=-$(NC)"
	@echo "$(YELLOW)Creating FAT32 image...$(NC)"
	dd if=/dev/zero of=build/VNiX-uefi_0.10.05.img bs=1080K count=64
	mkfs.fat -F 32 build/VNiX-uefi_0.10.05.img 2>/dev/null || sudo mkfs.fat -F 32 build/VNiX-uefi_0.10.05.img

# I decided to add a little interactive box because why not?
	@echo "$(CYAN)=-=-=-=-=-=-=-=-=-=-=-=-=-=-$(NC)"
	@echo "$(GREEN)Copying files to image...$(NC)"
	mcopy -i build/VNiX-uefi_0.10.05.img -s build/usb_root/* ::
	@echo ""
	@echo "$(MAGENTA)IMAGE CREATED at: build/VNiX-uefi_0.10.05.img$(NC)"
	@echo ""
	@echo "$(YELLOW)TO WRITE TO USB DRIVE:$(NC)"
	@echo "sudo dd if=build/VNiX-uefi_0.10.05.img of=/dev/sdX bs=4M status=progress"
	@echo "Or use a tool like balenaEtcher or Rufus to write the image onto a USB drive."
	@echo ""
	@echo "Replace /dev/sdX with your USB device (e.g: /dev/sdb)"
	@echo "Check with: lsblk or sudo fdisk -l"
	@echo ""
	@echo "$(RED)IMPORTANT: After writing, you MUST:$(NC)"
	@echo "1. Go to BIOS/UEFI settings"
	@echo "2. Disable Secure Boot"
	@echo "3. Set USB as first boot device "
	@echo "   (or press F9 or ESC for the boot menu if your machine supports it)"
	@echo ""
	@echo "Alternatively, you can run 'make run' to test the UEFI raw image in Qemu."
	@echo "===================================================================="
	@echo "Note: this image will NOT work for Non-UEFI/Legacy BIOS systems."
	qemu-img create -f qcow2 vnix.qcow2 10G
	@echo "===================================================================="
	@echo "created 10GB qcow2 hdd image"
	@echo ""

run:
# I think all devs know this by now but qemu can differ from real hardware, so dont rely on it 100%
	qemu-system-x86_64 $(if $(QEMU_CPU),-cpu $(QEMU_CPU)) \
		$(QEMU_MEM) \
		-bios boot/Assets/ovmf/OVMF.fd \
		-usb \
    	-device usb-ehci,id=ehci \
		-drive file=build/VNiX-uefi_0.10.05.img,format=raw \
		-serial stdio \
		-no-shutdown \
		-no-reboot \
		-accel kvm \
		-drive file=vnix.qcow2,format=qcow2,index=1

clean:
	@echo "$(CYAN)=-=-=-=-=-=-=-=-=-=-=-=-=-=-$(NC)"
	@echo "$(RED)Cleaning up files...$(NC)"
	rm -rf build