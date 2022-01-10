#include <stdint.h>
#include <aos/kernel.h>
#include <errno.h>

__attribute__((weak)) int usleep(useconds_t usec)
{
    if (usec && (usec <= 1000000)) {
        if (usec < 1000) {
            usec = 1000;
        }

        aos_msleep(usec / 1000);
        return 0;
    }

    return -EINVAL;
}