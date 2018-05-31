#include <cstddef>
#include <stdio.h>
#include "mem_debug.h"

extern "C" void *__real_malloc(size_t);
extern "C" void *__real_calloc(size_t nmemb,size_t);
extern "C" void *__real_realloc(void* ptr,size_t size);
extern "C" void __real_free(void*);


#define get_lr(reglr)	__asm__ __volatile__("mov %0, lr":"=r"(reglr)::"memory")

extern "C" void *__wrap_malloc(size_t c)
{
	int reglr = 0;

	get_lr(reglr);
	void* p = __real_malloc(c);
	mem_add_info((char*)p, c, reglr);

	return p;
}

extern "C" void __wrap_free(void* p)
{
	mem_del_info((char*)p);

	return __real_free(p);
}

extern "C" void *__wrap_calloc(size_t nmemb, size_t size)
{
	int reglr = 0;

	get_lr(reglr);
	void* p = __real_calloc(nmemb, size);
	mem_add_info((char*)p, nmemb * size, reglr);

	return p;
}

extern "C" void *__wrap_realloc(void *p, size_t size)
{
	int reglr = 0;

	get_lr(reglr);
	mem_del_info((char*)p);
	void* tmp = __real_realloc(p,size);
	mem_add_info((char*)tmp, size, reglr);

	return tmp;
}

void* operator new(size_t size)
{
	int reglr = 0;

	get_lr(reglr);
	void* p = __real_malloc(size);
	mem_add_info((char*)p, size, reglr);

	return p;
}

void operator delete(void *p)
{
	return __wrap_free(p);
}

void* operator new[](size_t size)
{
	int reglr = 0;

	get_lr(reglr);
	void* p = __real_malloc(size);
	mem_add_info((char*)p, size, reglr);

	return p;
}

void operator delete[](void* p)
{
	return __wrap_free(p);
}

