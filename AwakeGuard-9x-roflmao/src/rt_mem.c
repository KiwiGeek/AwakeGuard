/*
 * memcpy/memset/memmove/memcmp for the -nostdlib 9x build. GCC emits calls to
 * these for struct copies and zero-initialization even in freestanding code.
 * Compiled with -fno-builtin -fno-tree-loop-distribute-patterns (see
 * CMakeLists.txt) so GCC doesn't turn these loops back into calls to themselves.
 */
#include <stddef.h>

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dest;
}

void* memset(void* dest, int value, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    while (n--) {
        *d++ = (unsigned char)value;
    }
    return dest;
}

int memcmp(const void* a, const void* b, size_t n) {
    const unsigned char* x = (const unsigned char*)a;
    const unsigned char* y = (const unsigned char*)b;
    for (; n; --n, ++x, ++y) {
        if (*x != *y) {
            return *x < *y ? -1 : 1;
        }
    }
    return 0;
}
