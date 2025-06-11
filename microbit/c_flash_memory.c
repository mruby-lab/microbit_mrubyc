#include "mrubyc.h"
#include "nrf.h"

extern const uint8_t mrbbuf[];
extern const unsigned int mrbbuf_size;

// フラッシュページ消去
static void c_flash_erase(struct VM *vm, mrbc_value v[], int argc) {
  if (argc < 1 || v[1].tt != MRBC_TT_INTEGER) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "Expected Integer");
    return;
  }

  uint32_t addr = v[1].i;
  if (addr >= 0x00040000 || (addr % 1024)) {
    SET_INT_RETURN(1);
    return;
  }

  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
  NRF_NVMC->ERASEPAGE = addr;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

  SET_INT_RETURN(0);
}

// mrbbuf を Flash に書き込む
static void c_flash_write(struct VM *vm, mrbc_value v[], int argc) {
  if (argc < 1 || v[1].tt != MRBC_TT_INTEGER) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "Expected Integer");
    return;
  }

  uint32_t *addr = (uint32_t*)v[1].i;
  uint32_t len = (mrbbuf_size + 3) / 4;

  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

  for (uint32_t i = 0; i < len; i++) {
    uint32_t val = 0;
    for (int b = 0; b < 4; b++) {
      uint32_t byte = (4 * i + b < mrbbuf_size) ? mrbbuf[4 * i + b] : 0xFF;
      val |= byte << (8 * b);
    }
    *addr++ = val;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
  }

  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

  SET_INT_RETURN(0);
}

// フラッシュメモリの内容を読み出してシリアル出力する
static void c_flash_dump(struct VM *vm, mrbc_value v[], int argc) {
  if (argc < 2 || v[1].tt != MRBC_TT_INTEGER || v[2].tt != MRBC_TT_INTEGER) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "Expected addr and length");
    return;
  }

  uint32_t *addr = (uint32_t*)v[1].i;
  int length = v[2].i;

  for (int i = 0; i < length; i++) {
    uint32_t val = addr[i];
    char buf[20];
    sprintf(buf, "0x%08lx", val);
    my_print(buf);
    my_print("\n");
  }

  SET_NIL_RETURN();
}

// 初期化関数
int flash_memory_init(void) {
  mrbc_define_method(0, mrbc_class_object, "flash_erase", c_flash_erase);
  mrbc_define_method(0, mrbc_class_object, "flash_write", c_flash_write);
  mrbc_define_method(0, mrbc_class_object, "flash_dump", c_flash_dump);

  return 0;
}
