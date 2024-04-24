#include "mrubyc.h"

static void c_set_color(struct VM *vm, mrbc_value v[], int argc);
int robot_init(void);
static void c_move(struct VM *vm, mrbc_value v[], int argc);
static void c_track_control(struct VM *vm, mrbc_value v[], int argc);
