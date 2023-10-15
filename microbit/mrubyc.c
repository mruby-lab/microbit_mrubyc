// mruby/c boot

#include "mrubyc.h"

#if !defined(MRBC_MEMORY_SIZE)
#define MRBC_MEMORY_SIZE (1024*40)
#endif

static uint8_t memory_pool[MRBC_MEMORY_SIZE];

extern const uint8_t mrbbuf[];

int mrubyc(void){

  mrbc_init(memory_pool, MRBC_MEMORY_SIZE);

  if( mrbc_create_task(mrbbuf, 0) != NULL ){
    mrbc_run();
  }

  return 0;
}
