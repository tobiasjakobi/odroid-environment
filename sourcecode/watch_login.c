#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/time.h>

int i_fd = -1, i_w = -1;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cnd = PTHREAD_COND_INITIALIZER;

void* threadfunc(void *arg) {
  struct pollfd pf = {
    .fd = i_fd,
    .events = POLLIN
  };

  while (1) {
    pf.revents = 0;

    if (poll(&pf, 1, 1000) < 0)
      continue;

    if (pf.revents & (POLLHUP | POLLERR))
      continue;

    if (pf.revents & POLLIN)
      break;

    usleep(5000);
  }

  pthread_mutex_lock(&mtx);
  pthread_cond_signal(&cnd);
  pthread_mutex_unlock(&mtx);

  pthread_exit(0);
}

int main(int argc, char* argv[]) {
  struct timespec tw;
  pthread_t thread;
  int ret;

  if (argc < 2)
    return 1;

  i_fd = inotify_init();
  if (i_fd < 0)
    return 2;

  i_w = inotify_add_watch(i_fd, argv[1], IN_MODIFY);
  if (i_w < 0) {
    close(i_fd);
    return 3;
  }

  ret = pthread_create(&thread, NULL, threadfunc, NULL);
  if (ret < 0)
    goto out;

  gettimeofday(&tw, NULL);
  tw.tv_sec += 3 * 60;

  ret = pthread_cond_timedwait(&cnd, &mtx, &tw);

  if (ret == ETIMEDOUT) {
    pthread_cancel(thread);
    ret = 4;
  } else {
    void *tmp;
    pthread_join(thread, &tmp);
    ret = 0;
  }

out:
  inotify_rm_watch(i_fd, i_w);
  close(i_fd);

  return ret;
}
