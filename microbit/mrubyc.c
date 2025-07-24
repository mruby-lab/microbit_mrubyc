/**
 * @file mrubyc.c
 * @brief mruby/cの初期化と、C言語で実装されたカスタムメソッドの登録
 * @details C++依存のコードを完全に分離し、.ino側で定義された
 * C言語互換のラッパー関数を呼び出す。
 */

#include "mrubyc.h"
#include "c_robot.h"
#include "c_flash_memory.h"
#include "c_gpio.h"
#include <nrf.h>

// .ino側で実装されるC言語互換のラッパー関数のプロトタイプ宣言
#ifdef __cplusplus
extern "C" {
#endif

void c_if_display_show(const char* image_data);

#ifdef __cplusplus
}
#endif


#if !defined(MRBC_MEMORY_SIZE)
#define MRBC_MEMORY_SIZE (1024*40)
#endif

static uint8_t memory_pool[MRBC_MEMORY_SIZE];

extern uint8_t mrbbuf_ram[];


static void c_message(struct VM *vm, mrbc_value v[], int argc){
  if( argc == 0 ){
    my_print("message method in mruby/c\n");
  } else {
    if( argc == 1 && v[1].tt == MRBC_TT_INTEGER ){
      int n = v[1].i;
      my_print("message: ");
      while( n > 0 ){
        my_print("*");
        n--;
      }
      my_print("\n");
    }
  }
}

// Rubyの `display_show` 命令を処理するC言語関数
static void c_display_show(mrbc_vm *vm, mrbc_value v[], int argc) {
  if (argc == 1 && v[1].tt == MRBC_TT_STRING) {
    const char* image_data = mrbc_string_cstr(&v[1]);
    // .ino側で実装されたラッパー関数に処理を渡す
    c_if_display_show(image_data);
  }
  SET_NIL_RETURN();
}


int flash_write_data(uint32_t addr, const uint8_t *data, int len) {
  if (addr % 1024 != 0 || addr >= 0x00040000) return 1;
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
  for (int i = 0; i < len; i += 4) {
    uint32_t word = 0xFFFFFFFF;
    for (int j = 0; j < 4 && (i + j) < len; j++) {
      word &= ~(0xFF << (8 * j));
      word |= data[i + j] << (8 * j);
    }
    *((uint32_t*)(addr + i)) = word;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
  }
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
  return 0;
}
int flash_erase_page(uint32_t addr) {
  if (addr % 1024 != 0 || addr >= 0x00040000) return 1;
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
  NRF_NVMC->ERASEPAGE = addr;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
  return 0;
}



int mrubyc(void){

  my_print("start mruby/c\n");

  mrbc_init(memory_pool, MRBC_MEMORY_SIZE);
  
  // カスタムメソッドの登録
  mrbc_define_method(0, mrbc_class_object, "message", c_message);
  mrbc_define_method(0, mrbc_class_object, "display_show", c_display_show);

  robot_init();
  flash_memory_init();
  gpio_init();

  if( !mrbc_create_task(mrbbuf_ram, NULL) ) return 1;
  int ret = mrbc_run();

  return 0;
}
