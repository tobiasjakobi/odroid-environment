#include <unistd.h>
#include <linux/reboot.h>
#include <sys/syscall.h>
#include <sys/types.h>

int main()
{
  sync();
  syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART, NULL);
}
