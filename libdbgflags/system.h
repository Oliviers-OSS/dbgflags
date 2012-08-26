#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <pthread.h>
#include <linux/unistd.h>
#include <linux/version.h>
#include <errno.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
static __inline pid_t __gettid() {
  long res = 0;

  /* linux 2.4.x'syscall using int $0x80 */
  __asm__ volatile ("int $0x80" \
  : "=a" (res) \
  : "0" (__NR_gettid));
  do
  {
    if ((unsigned long)(res) >= (unsigned long)(-(128 + 1)))
    {
      errno = -(res);
      res = -1;
    }
    return (pid_t) (res);
  } while (0);
  return res;
}
#else //(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
#include <sys/syscall.h>

static __inline pid_t __gettid() {
  const pid_t tid = (pid_t) syscall (SYS_gettid);
  return tid;
}

#endif //(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))

#define gettid __gettid

static __inline unsigned long long int rdtsc(void) {
   unsigned long long int x;
   unsigned a, d;

   __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

   return ((unsigned long long)a) | (((unsigned long long)d) << 32);;
}

#endif /* _SYSTEM_H_ */


