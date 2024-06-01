#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int ret;

  fprintf(2, "Changing priority\n");
  ret = set_priority(NLEVELS - 1);
  fprintf(2, "set_priority exited with code %d\n", ret);
  fprintf(2, "Priority level was changed to %d\n", get_priority());

  fprintf(2, "\n");

  fprintf(2, "Changing to priority out of range\n");
  ret = set_priority(NLEVELS);
  fprintf(2, "set_priority exited with code %d\n", ret);

  return 0;
}