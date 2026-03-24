# --- Herramientas de Compilacion (Toolchain) ---
CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

# --- Banderas de Compilacion ---
CFLAGS = -mcpu=cortex-a8 -marm -Wall -O0 -ffreestanding -nostdlib -I./lib -I./hal -I./os
LDFLAGS = -nostdlib

# --- Directorio de Salida ---
BUILD_DIR = build

# --- Archivos Objeto Compartidos (Libreria y Hardware) ---
# (Si tienes lib/string.c agregalo aqui como $(BUILD_DIR)/lib/string.o)
SHARED_OBJS = $(BUILD_DIR)/lib/stdio.o $(BUILD_DIR)/hal/uart.o 

# --- Archivos Objeto del OS ---
OS_OBJS = $(BUILD_DIR)/os/root.o \
          $(BUILD_DIR)/os/os.o \
          $(BUILD_DIR)/os/scheduler.o \
          $(BUILD_DIR)/hal/watchdog.o \
          $(BUILD_DIR)/hal/timer.o \
          $(SHARED_OBJS)

# --- Archivos Objeto de los Procesos de Usuario ---
P1_OBJS = $(BUILD_DIR)/P1/main.o $(SHARED_OBJS)
P2_OBJS = $(BUILD_DIR)/P2/main.o $(SHARED_OBJS)

# --- Regla Principal (Lo que se hace al escribir 'make') ---
all: $(BUILD_DIR)/os.bin $(BUILD_DIR)/p1.bin $(BUILD_DIR)/p2.bin

# --- Reglas Magicas para compilar y crear subcarpetas en build/ ---
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Reglas de Construccion del OS ---
$(BUILD_DIR)/os.elf: $(OS_OBJS)
	$(LD) $(LDFLAGS) -T os/os.ld $^ -o $@

$(BUILD_DIR)/os.bin: $(BUILD_DIR)/os.elf
	$(OBJCOPY) -O binary $< $@

# --- Reglas de Construccion de P1 ---
$(BUILD_DIR)/p1.elf: $(P1_OBJS)
	$(LD) $(LDFLAGS) -T P1/p1.ld $^ -o $@

$(BUILD_DIR)/p1.bin: $(BUILD_DIR)/p1.elf
	$(OBJCOPY) -O binary $< $@

# --- Reglas de Construccion de P2 ---
$(BUILD_DIR)/p2.elf: $(P2_OBJS)
	$(LD) $(LDFLAGS) -T P2/p2.ld $^ -o $@

$(BUILD_DIR)/p2.bin: $(BUILD_DIR)/p2.elf
	$(OBJCOPY) -O binary $< $@

# --- Regla de Limpieza ---
clean:
	rm -rf $(BUILD_DIR)