#ifndef C_FLASH_MEMORY_H
#define C_FLASH_MEMORY_H

#include "mrubyc.h"

static void c_flash_erase(struct VM *vm, mrbc_value v[], int argc);
static void c_flash_write(struct VM *vm, mrbc_value v[], int argc);
static void c_flash_dump(struct VM *vm, mrbc_value v[], int argc);
int flash_memory_init(void);

#endif
