#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <pthread.h>
#include <linux/unistd.h>
#include <errno.h>

static pid_t __gettid()
{
  long res;
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
}
#define gettid __gettid

static __inline unsigned long long int rdtsc(void)
{
   unsigned long long int x;
   unsigned a, d;

   __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

   return ((unsigned long long)a) | (((unsigned long long)d) << 32);;
}

#endif /* _SYSTEM_H_ */


