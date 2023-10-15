#include "hal.h"

int hal_write(int fd, const void *buf, int nbytes)
{
  int i;
  for( i=0 ; i<nbytes ; i++ ){
    char *p = (const char *)(buf+i);
    my_putc(*p);
  }
  return nbytes;
}

int hal_flush(int fd)
{
  return 0;
}


void hal_abort(const char *s)
{
}
