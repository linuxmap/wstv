#ifndef _MEM_DEBUG_H_
#define _MEM_DEBUG_H_

#ifdef __cplusplus
#define MEM_API		extern "C"
#else
#define MEM_API
#endif

MEM_API void mem_add_info(char* p, size_t nSize, int lr);
MEM_API void mem_del_info(char* p);

MEM_API void RegMemCmd(void);

#endif

