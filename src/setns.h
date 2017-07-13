#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 14
/*  Define setns() if missing from the C library */
#include <error.h>
static inline int setns(int fd, int nstype)
{
#ifdef __NR_setns
    return syscall(__NR_setns, fd, nstype);
#elif defined(__NR_set_ns)
    return syscall(__NR_set_ns, fd, nstype);
#else
    errno = ENOSYS;
    return -1;
#endif
}
#endif
