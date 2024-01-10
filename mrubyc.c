// mruby/c boot

#include "mrubyc.h"
#include "c_robot.h"


#if !defined(MRBC_MEMORY_SIZE)
#define MRBC_MEMORY_SIZE (1024*40)
#endif

static uint8_t memory_pool[MRBC_MEMORY_SIZE];

extern const uint8_t mrbbuf[];


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





int mrubyc(void){

  my_print("start mruby/c\n");

  mrbc_init(memory_pool, MRBC_MEMORY_SIZE);
  

  // Define your own class.
  mrbc_define_method(0, mrbc_class_object, "message", c_message);
  robot_init();


  if( !mrbc_create_task(mrbbuf, NULL) ) return 1;
  int ret = mrbc_run();

  return 0;
}
