# beagle por defecto, pero se puede cambiar a QEMU con "make TARGET=QEMU"
TARGET ?= BEAGLE

# Herramientas para compilar
CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

# CONFIG del condicional (BSP)
ifeq ($(TARGET), QEMU)
    HW_DIR = hal/qemu
    CFLAGS = -mcpu=arm926ej-s -marm -Wall -O0 -ffreestanding -nostdlib -I./lib -I./hal -I./os -D QEMU_MODE
	OS_LD = hal/qemu/qemu.ld  
	P1_LD = hal/qemu/qemu_p1.ld
    P2_LD = hal/qemu/qemu_p2.ld
else
    HW_DIR = hal/beagle
    CFLAGS = -mcpu=cortex-a8 -marm -Wall -O0 -ffreestanding -nostdlib -I./lib -I./hal -I./os
	OS_LD = hal/beagle/beagle.ld
	P1_LD = P1/p1.ld
    P2_LD = P2/p2.ld
endif

LDFLAGS = -nostdlib
BUILD_DIR = build

# Archivos Objeto
# Nota como usamos $(HW_DIR) para jalar los archivos correctos
SHARED_OBJS = $(BUILD_DIR)/lib/stdio.o $(BUILD_DIR)/$(HW_DIR)/uart.o 

OS_OBJS = $(BUILD_DIR)/$(HW_DIR)/root.o \
          $(BUILD_DIR)/os/os.o \
          $(BUILD_DIR)/os/scheduler.o \
          $(BUILD_DIR)/$(HW_DIR)/watchdog.o \
          $(BUILD_DIR)/$(HW_DIR)/timer.o \
          $(SHARED_OBJS)

P1_OBJS = $(BUILD_DIR)/P1/main.o $(SHARED_OBJS)
P2_OBJS = $(BUILD_DIR)/P2/main.o $(SHARED_OBJS)

# regla principal
all: $(BUILD_DIR)/os.bin $(BUILD_DIR)/p1.bin $(BUILD_DIR)/p2.bin

# Reglas de Compilacion (Generan las carpetas automaticas) ---
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# reglas para el linkeo
$(BUILD_DIR)/os.elf: $(OS_OBJS)
	$(LD) $(LDFLAGS) -T $(OS_LD) $^ -o $@

$(BUILD_DIR)/os.bin: $(BUILD_DIR)/os.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/p1.elf: $(P1_OBJS)
	$(LD) $(LDFLAGS) -T P1/p1.ld $^ -o $@

$(BUILD_DIR)/p1.bin: $(BUILD_DIR)/p1.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/p2.elf: $(P2_OBJS)
	$(LD) $(LDFLAGS) -T P2/p2.ld $^ -o $@

$(BUILD_DIR)/p2.bin: $(BUILD_DIR)/p2.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

# --- Comandos extra ---
clean:
	rm -rf $(BUILD_DIR)

# Comando rapido para correr en QEMU
run-qemu:
	qemu-system-arm -M versatilepb -nographic \
	  -kernel build/os.elf \
	  -device loader,file=build/p1.bin,addr=0x00100000 \
	  -device loader,file=build/p2.bin,addr=0x00200000