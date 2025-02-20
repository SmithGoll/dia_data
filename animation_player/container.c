#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>

#if defined(CPU_RELAX)
    #define cpu_relax CPU_RELAX
#elif (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
    #define cpu_relax() __asm__ __volatile__("pause\n")
#elif (defined(__arm__) && defined(__ARM_ARCH) && __ARM_ARCH >= 7) || defined(__aarch64__)
    #define cpu_relax() __asm__ __volatile__("yield" ::: "memory")
#elif (defined(__powerpc__) || defined(__powerpc64__))
    #define cpu_relax() __asm__ __volatile__("or 27,27,27");
#else
    #define cpu_relax() /* nothing */
#endif

typedef struct
{
    atomic_flag lock;
    size_t count;
    void *data[0];
} container_t;

static void container_lock(container_t *container)
{
    while(atomic_flag_test_and_set(&(container->lock)))
    {
        cpu_relax();
    }
}

static void container_unlock(container_t *container)
{
    atomic_flag_clear(&(container->lock));
}

static int container_check_bound(const container_t *container, size_t index)
{
    if(!container) return 0;
    if(container->count <= index) return 0;
    return 1;
}

container_t *container_alloc(size_t count)
{
    if(!count) return NULL;

    container_t *res = (container_t*)malloc(sizeof(container_t) + (count * sizeof(void*)));
    if(!res) return NULL;

    res->count = count;
    for(int i = 0; i < count; i++)
        res->data[i] = NULL;

    container_unlock(res);
    return res;
}

size_t container_get_size(container_t *container)
{
    container_lock(container);

    size_t size;
    if(!container_check_bound(container, 0)) return 0;
    size = container->count;
    container_unlock(container);
    return size;
}

void *container_get(container_t *container, size_t index)
{
    container_lock(container);

    void *res;
    if(!container_check_bound(container, index)) return NULL;
    res = container->data[index];

    container_unlock(container);
    return res;
}

void container_put(container_t *container, void *in, size_t index)
{
    container_lock(container);

    if(!container_check_bound(container, index)) return;
    container->data[index] = in;

    container_unlock(container);
    return;
}
