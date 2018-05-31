#ifndef _UTL_MAP_H_
#define _UTL_MAP_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef void*	MAP_HDL;

MAP_HDL utl_map_create();

int utl_map_add_pair(MAP_HDL map, const char* key, const char* value);

int utl_map_remove_pair(MAP_HDL map, const char* key);

int utl_map_sort(MAP_HDL map);

const char* utl_map_get_val(MAP_HDL map, const char* key);

void utl_map_clear(MAP_HDL map);

int utl_map_generate_value(const MAP_HDL map, char* dst, int len, const char* conn);

int utl_map_generate_string(const MAP_HDL map, char* dst, int len, const char* conn_kv, const char* conn_p);

void utl_map_destory(MAP_HDL map);
	
#ifdef __cplusplus
}
#endif

#endif

