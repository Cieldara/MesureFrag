#include "mem.h"
#include "common.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/*
static __thread int in_lib=0;
#define dprintf(args...) \
  do { \
    if (!in_lib) { \
      in_lib=1; \
      printf(args); \
      in_lib=0; \
    } \
  } while (0)
*/

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void init() {
    static int first=1;

    if (first) {
        mem_init(get_memory_adr(), get_memory_size());
        first = 0;
    }
}

void *malloc(size_t s) {
	pthread_mutex_lock (&mutex);
	void *result;

	init();
	result = mem_alloc(s);
	/*
	if (!result)
		dprintf(" Alloc FAILED !!");
	else
		dprintf("Allocation de %lu octets... en %lx\n", (unsigned long) s, (unsigned long) result);
	*/
	pthread_mutex_unlock (&mutex);
	return result;
}

void *calloc(size_t count, size_t size) {
	int i;
	char *result;
	size_t s = count*size;

	init();
	result = mem_alloc(s);
	if (result){
		for (i=0; i<s; i++){
			//dprintf("Allocation de %lu octets... en %lx\n", (unsigned long) s, (unsigned long) result[i]);
			result[i] = 0;
		}
	} /* 
	else {
		dprintf(" Alloc FAILED !!");
	} */
	pthread_mutex_unlock (&mutex);
	return result;
}

void *realloc(void *ptr, size_t size) {
	pthread_mutex_lock (&mutex);
	size_t s;
	char *result;

	init();
	//dprintf("Reallocation de la zone en %lx\n", (unsigned long) ptr);
	if (!ptr) {
		//dprintf(" Realloc of NULL pointer\n");
		pthread_mutex_unlock (&mutex);
		return mem_alloc(size);
	}
	if (mem_get_size(ptr) >= size) {
		//dprintf(" Useless realloc\n");
		pthread_mutex_unlock (&mutex);
		return ptr;
	}
	result = mem_alloc(size);
	if (!result) {
		//dprintf(" Realloc FAILED\n");
		pthread_mutex_unlock (&mutex);
		return NULL;
	}
	for (s = 0; s<mem_get_size(ptr); s++)
		result[s] = ((char *) ptr)[s];
	mem_free(ptr);
	//dprintf(" Realloc ok\n");
	pthread_mutex_unlock (&mutex);
	return result;
}

void free(void *ptr) {
	pthread_mutex_lock (&mutex);
	init();
	if (ptr) {
		//dprintf("Liberation de la zone en %lx\n", (unsigned long) ptr);
		mem_free(ptr);
	} /*
	else {
		dprintf("Liberation de la zone NULL\n");
	} */
	pthread_mutex_unlock (&mutex);
}
