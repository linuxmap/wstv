#ifndef KS_API_H
#define KS_API_H

#include "strmap.h"

#ifdef __cplusplus
extern "C"
{
#endif

int ks_init(const char* id, const char* key, char* response);

int ks_list();

int ks_list_bucket(char* bucket_name, char* args);

int ks_create_bucket(char* bucket_name, StrMap* map, char* args);

int ks_create_object(char* bucket_name, char* object_key, char* object_data, StrMap* map, char* args);

int ks_create_object_file(char* bucket_name, char* object_key, char* filename, StrMap* map, char* args);

int ks_download_object_file(char* bucket_name, char* object_key, char* filename, StrMap* map, char* args);

int ks_download_object(char* bucket_name, char* object_key, StrMap* map, char* args);

int ks_delete_object(char* bucket_name, char* object_key, StrMap* map, char* args);

int ks_delete_bucket(char* bucket_name, StrMap* map, char* args);

int ks_create_object_file_md5(char* bucket_name, char* object_key, char* filename, StrMap* map, char* args);

#ifdef __cplusplus
}
#endif

#endif
