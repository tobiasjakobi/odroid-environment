#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/reboot.h>

int main(int argc, char* argv[]) {
  char buf[256];

  const unsigned num_labels = argc - 1;
  const unsigned sleepus = 500000;
  const unsigned maxwait = 10;

  unsigned i, j;
  int fd;

  if (num_labels == 0)
    return 0;

  for (i = 0; i < num_labels; ++i) {
    snprintf(buf, sizeof(buf), "/dev/disk/by-label/%s", argv[i + 1]);

    fd = -1;
    j = 0;
    while (fd < 0) {
      if (j > maxwait) goto fail;

      usleep(sleepus);
      fd = open(buf, O_RDONLY);

      ++j;
    }

    close(fd);
  }

  return 0;

fail:
  if (fd >= 0) close(fd);

  sync();
  syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_HALT, NULL);

  return 1;
}
