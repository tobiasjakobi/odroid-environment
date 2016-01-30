#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <pthread.h>

enum threadstatus {
  thread_initialized = 0,
  thread_ready = 1,
  thread_exit = 2
};

struct threadctrl {
  unsigned *free_threads;
  unsigned num_free_threads;

  pthread_mutex_t mtx;
  pthread_cond_t cnd;
};

struct threaddata {
  unsigned index;
  enum threadstatus status;

  int work_time;

  pthread_mutex_t mtx;
  pthread_cond_t cnd;

  struct threadctrl *ctrl;
};

void threadctrl_put(struct threadctrl* ctrl, unsigned index) {
  ctrl->free_threads[ctrl->num_free_threads] = index;
  ++(ctrl->num_free_threads);
}

unsigned threadctrl_get(struct threadctrl* ctrl) {
  --(ctrl->num_free_threads);
  return ctrl->free_threads[ctrl->num_free_threads];
}

void* threadfunc(void *arg) {
  struct threaddata *data;
  struct threadctrl *ctrl;

  data = arg;
  ctrl = data->ctrl;

  while (true) {
    pthread_mutex_lock(&data->mtx);
    data->status = thread_ready;
    pthread_cond_wait(&data->cnd, &data->mtx);
    
    if (data->status == thread_exit) break;

    fprintf(stderr, "thread %u working for %u microseconds\n",
            data->index, data->work_time);
    usleep(data->work_time);

    pthread_mutex_lock(&ctrl->mtx);

    threadctrl_put(ctrl, data->index);

    pthread_cond_signal(&ctrl->cnd);
    pthread_mutex_unlock(&ctrl->mtx);
    
    pthread_mutex_unlock(&data->mtx);
  }

  pthread_exit(0);
}

int main(int argc, char* argv[]) {
  const unsigned num_threads = 4;

  pthread_t threads[num_threads];
  struct threaddata data[num_threads];

  unsigned i;

  /* thread control */
  struct threadctrl ctrl = {
    .free_threads = malloc(num_threads * sizeof(unsigned)),
    .num_free_threads = 0,

    .mtx = PTHREAD_MUTEX_INITIALIZER,
    .cnd = PTHREAD_COND_INITIALIZER
  };

  /* initialize */
  srand(time(NULL));
  for (i = 0; i < num_threads; ++i) {
    data[i].index = i;
    data[i].status = thread_initialized;

    pthread_mutex_init(&data[i].mtx, NULL);
    pthread_cond_init(&data[i].cnd, NULL);

    threadctrl_put(&ctrl, i);
    data[i].ctrl = &ctrl;

    pthread_create(&threads[i], NULL, threadfunc, &data[i]);

    /* wait until thread is ready */
    while (data[i].status != thread_ready)
      usleep(500);
  }
  
  for (i = 0; i < num_threads * 16; ++i) {
    unsigned k;

    /* find free thread, otherwise wait */
    pthread_mutex_lock(&ctrl.mtx);

    if (ctrl.num_free_threads == 0)
      pthread_cond_wait(&ctrl.cnd, &ctrl.mtx);

    k = threadctrl_get(&ctrl);

    pthread_mutex_unlock(&ctrl.mtx);

    /* configure thread data and signal it to work */
    pthread_mutex_lock(&data[k].mtx);
    data[k].work_time = rand() % 100000;
    pthread_cond_signal(&data[k].cnd);
    pthread_mutex_unlock(&data[k].mtx);
  }

  /* signal all threads to exit */
  for (i = 0; i < num_threads; ++i) {
    pthread_mutex_lock(&data[i].mtx);
    data[i].status = thread_exit;
    pthread_cond_signal(&data[i].cnd);
    pthread_mutex_unlock(&data[i].mtx);
  }

  /* wait for threads to exit */
  for (i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], NULL);
  }

  free(ctrl.free_threads);
}
