#include <map>
#include <string>
#include <cstring>
#include <cstdio>

#include "utl_map.h"

typedef std::map<std::string, std::string> MAP_t;

MAP_HDL utl_map_create()
{
	return new std::map<std::string, std::string>;
}

int utl_map_add_pair(MAP_HDL map, const char* key, const char* value)
{
	if (!map)
	{
		return -1;
	}

	MAP_t* Map = (MAP_t*)map;

	// Map->insert(std::pair<std::string, std::string>(key, value));
	(*Map)[key] = value;

	return 0;
}

int utl_map_remove_pair(MAP_HDL map, const char* key)
{
	if (!map)
	{
		return -1;
	}

	MAP_t* Map = (MAP_t*)map;

	Map->erase(key);

	return 0;
}

int utl_map_sort(MAP_HDL map)
{
	return 0;
}

void utl_map_clear(MAP_HDL map)
{
	if (!map)
	{
		return;
	}

	MAP_t* Map = (MAP_t*)map;

	return Map->clear();
}

const char* utl_map_get_val(MAP_HDL map, const char* key)
{
	if (!map)
	{
		return NULL;
	}

	MAP_t* Map = (MAP_t*)map;

	return (*Map)[key].c_str();
}

int utl_map_generate_value(const MAP_HDL map, char* dst, int len, const char* conn)
{
	if (!map || !dst || !len || !conn)
	{
		return -1;
	}

	const MAP_t* Map = (const MAP_t*)map;
	int nLen = 0;
	int conn_len = strlen(conn);

	std::map<std::string, std::string>::const_iterator itr;
	for (itr = Map->begin(); itr != Map->end(); ++itr)
	{
		if (nLen == 0)
		{
			if (len <= nLen + (int)itr->second.length())
			{
				break;
			}
				
			nLen += snprintf(dst + nLen, len - nLen, "%s", itr->second.c_str());
		}
		else
		{
			if (len <= nLen + conn_len + (int)itr->second.length())
			{
				break;
			}

			nLen += snprintf(dst + nLen, len - nLen, "%s%s", conn, itr->second.c_str());
		}
	}

	return nLen;
}

int utl_map_generate_string(const MAP_HDL map, char* dst, int len, const char* conn_kv, const char* conn_p)
{
	if (!map || !dst || !len || !conn_kv || !conn_p)
	{
		return -1;
	}

	const MAP_t* Map = (const MAP_t*)map;
	int nLen = 0;
	int conn_kv_len = strlen(conn_kv);
	int conn_p_len = strlen(conn_p);
	

	std::map<std::string, std::string>::const_iterator itr;
	for (itr = Map->begin(); itr != Map->end(); ++itr)
	{
		if (nLen == 0)
		{
			if (len <= nLen + (int)itr->first.length() + conn_kv_len + (int)itr->second.length())
			{
				break;
			}
			
			nLen += snprintf(dst + nLen, len - nLen, "%s%s%s", itr->first.c_str(), conn_kv, itr->second.c_str());
		}
		else
		{
			if (len <= nLen + conn_p_len + (int)itr->first.length() + conn_kv_len + (int)itr->second.length())
			{
				break;
			}

			nLen += snprintf(dst + nLen, len - nLen, "%s%s%s%s", conn_p, itr->first.c_str(), conn_kv, itr->second.c_str());
		}
	}

	return nLen;
}

void utl_map_destory(MAP_HDL map)
{
	if (!map)
	{
		return;
	}

	MAP_t* Map = (MAP_t*)map;

	delete Map;	
}


