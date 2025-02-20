#include <stddef.h>

#ifndef _CONTAINER_H_
#define _CONTAINER_H_

struct container_s;
typedef struct container_s container_t;

container_t *container_alloc(size_t count);
size_t container_get_size(container_t *container);
void *container_get(container_t *container, size_t index);
void container_put(container_t *container, void *in, size_t index);

#endif // _CONTAINER_H_
