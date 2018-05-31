#include "utl_inifile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <ctype.h>

// #define _DEBUG
#ifdef _DEBUG
#define DBG_INFO(fmt...)  \
		do { \
				printf(fmt); \
			}while(0)
#else
#define DBG_INFO(fmt...)
#endif


static pthread_mutex_t iniMutex = PTHREAD_MUTEX_INITIALIZER;

static int __ini_lock()
{
	return pthread_mutex_lock(&iniMutex);
}

static int __ini_unlock()
{
	return pthread_mutex_unlock(&iniMutex);
}

#define INI_LOCK() __ini_lock()
#define INI_UNLOCK() __ini_unlock()


static char *str_triml(char *pstr)
{
	if (pstr == NULL)
	{
		return NULL;
	}

	char *lpos = pstr;
	while (isspace(*lpos))
	{
		lpos++;
	}
	if (pstr != lpos)
	{
		memmove(pstr, lpos, strlen(lpos) + 1);
	}
	return pstr;
}

static char *str_trimr(char *pstr)
{
	if (pstr == NULL || *pstr == '\0') 
		return pstr;

	char *rpos = pstr + strlen(pstr) - 1;
	while (rpos >= pstr && isspace(*rpos))
	{
		rpos--;
	}

	*(rpos + 1) = '\0';
	return pstr;
}

static char *str_trimlr(char *pstr)
{
#if 1
	str_triml(pstr);
	str_trimr(pstr);
	return pstr;
#else
	char* lpos;		/*左面第一个非空字符指针*/
	char* rpos;		/*右面第一个非空字符指针*/

	if (pstr == NULL) return NULL;

	lpos = pstr;
	while (isspace(*lpos)) ++lpos;
	rpos = lpos + strlen(lpos) - 1;
	while (rpos >= lpos&&isspace(*rpos)) --rpos;
	*(rpos + 1) = '\0';
	if (pstr != lpos) memmove(pstr, lpos, 2 + rpos - lpos);
	return pstr;
#endif
}

static int inifile_parse(char *strbuf, INIFILE *inifile)
{
	char *subBuf;
	char *begin;
	char *end;
	char *ptr;
	char *key, *value;
	INIFILE_SECTION *new_section = NULL;
	INIFILE_SECTION *last_section = NULL;
	INIFILE_KEY *new_key = NULL;
	INIFILE_KEY *last_key = NULL;

	if(strbuf==NULL || inifile==NULL)
	{
		return -1;
	}

	inifile->sectionList = NULL;
	subBuf = strbuf;
	str_trimlr(strbuf);

	while(subBuf)
	{
		end = strchr(subBuf, '\n');
		if(end)
		{
			begin = subBuf;
			subBuf = end + 1;
			*end = '\0';
			str_trimlr(begin);
			end = begin + strlen(begin);
		}
		else
		{
			begin = subBuf;
			subBuf = NULL;
			str_trimlr(begin);
			end = begin + strlen(begin);
		}

		if(*begin=='[' && *(end-1)==']' ) //&& strlen(begin)>=3)
		{
			begin++;
			*(end-1) = '\0';
			str_trimlr(begin);
			new_section = (INIFILE_SECTION *)malloc(sizeof(INIFILE_SECTION));
			if (new_section == NULL)
			{
				return -1;
			}
			snprintf(new_section->name, sizeof(new_section->name), "%s", begin);
			new_section->keyList = NULL;
			new_section->next = NULL;
			if (inifile->sectionList == NULL)
			{
				inifile->sectionList = new_section;
			}
			else
			{
				last_section->next = new_section;
			}
			last_section = new_section;
		}
		else
		{
			ptr = strchr(begin, '=');
			if(ptr)
			{
				*ptr++ = '\0';
				key = begin;
				value = ptr;
				str_trimlr(key);
				str_trimlr(value);

				new_key = (INIFILE_KEY *)malloc(sizeof(INIFILE_KEY));
				if (new_key == NULL)
				{
					return -1;
				}
				snprintf(new_key->key, sizeof(new_key->key), "%s", key);
				snprintf(new_key->value, sizeof(new_key->value), "%s", value);
				new_key->next = NULL;
				if (new_section->keyList == NULL)
				{
					new_section->keyList = new_key;
				}
				else
				{
					last_key->next = new_key;
				}
				last_key = new_key;
			}
		}
	}

	return 0;
}

static void inifile_print(INIFILE *inifile)
{
	INI_LOCK();
	INIFILE_SECTION *cur_section = NULL;
	INIFILE_KEY *cur_key = NULL;

	cur_section = inifile->sectionList;
	while (cur_section)
	{
		cur_key = cur_section->keyList;
		while (cur_key)
		{
			cur_key = cur_key->next;
		}
		cur_section = cur_section->next;
	}
	INI_UNLOCK();
}

int inifile_init(const char *fileName, INIFILE *inifile)
{
	INI_LOCK();
	int ret;
	int fileSize;
	FILE *fp;
	struct stat file_stat;
	unsigned char *buf = NULL;

	if(inifile == NULL)
	{
		INI_UNLOCK();
		return -1;
	}
	
	inifile->sectionList = NULL;

	if (stat(fileName, &file_stat) < 0)
	{
		DBG_INFO("fstat[%s] fail\n", fileName);
		INI_UNLOCK();
		return -1;
	}

	fileSize = file_stat.st_size;
	
	fp = fopen(fileName, "rb");
	if (fp == NULL)
	{
		DBG_INFO("fopen [%s] fail\n", fileName);
		INI_UNLOCK();
		return -1;
	}

	buf = (unsigned char *)malloc(fileSize+1);
	if(buf == NULL)
	{
		fclose(fp);
		INI_UNLOCK();
		return -1;
	}

	if((ret=fread(buf, 1, fileSize, fp)) != fileSize)
	{
		fclose(fp);
		free(buf);
		DBG_INFO("fread file[%s]fail: has[%d],read[%d]\n", fileName, fileSize, ret);
		INI_UNLOCK();
		return -1;
	}
	fclose(fp);
	*(buf+fileSize) = '\0';
	ret = inifile_parse((char*)buf, inifile);
	free(buf);
	if(ret)
	{
		inifile_free(inifile);
	}
	INI_UNLOCK();
	return ret;
}

void inifile_free(INIFILE *inifile)
{
	INI_LOCK();
	INIFILE_SECTION *cur_section;
	INIFILE_KEY *cur_key;

	if(inifile == NULL)
	{
		INI_UNLOCK();
		return;
	}

	while (inifile->sectionList)
	{
		cur_section = inifile->sectionList;
		inifile->sectionList = cur_section->next;
		while (cur_section->keyList)
		{
			cur_key = cur_section->keyList;
			cur_section->keyList = cur_key->next;;
			free(cur_key);
		}
		free(cur_section);
	}

	INI_UNLOCK();
	return;
}

int inifile_get(INIFILE *inifile, char *section, char *key, char *value, int size)
{
	INI_LOCK();
	INIFILE_SECTION *cur_section = NULL;
	INIFILE_KEY *cur_key = NULL;
	int len = 0;

	if (inifile == NULL || section == NULL || key == NULL || value == NULL || size <= 0)
	{
		INI_UNLOCK();
		return -1;
	}

	*value = '\0';

	char sectionName[32];
	char keyName[32];
	snprintf(sectionName, sizeof(sectionName), "%s", section);
	snprintf(keyName, sizeof(keyName), "%s", key);
	str_trimlr(sectionName);
	str_trimlr(keyName);

	cur_section = inifile->sectionList;
	while (cur_section)
	{
		if (strcasecmp(sectionName, cur_section->name) == 0)
		{
			cur_key = cur_section->keyList;
			while (cur_key)
			{
				if (strcasecmp(keyName, cur_key->key) == 0)
				{
					len = strlen(cur_key->value) + 1;
					if (len > size)
					{
						DBG_INFO("buffer size is too small: %d < %d\n", size, len);
						INI_UNLOCK();
						return -1;
					}
					snprintf(value, size, "%s", cur_key->value);
					INI_UNLOCK();
					return 0;
				}
				cur_key = cur_key->next;
			}
			break;
		}
		cur_section = cur_section->next;
	}

	DBG_INFO("inifile get fail: section[%s], key[%s]\n", sectionName, keyName);
	INI_UNLOCK();
	return -1;
}

int inifile_put(INIFILE *inifile, char *section, char *key, char *value)
{
	INI_LOCK();
	INIFILE_SECTION *cur_section = NULL;
	INIFILE_SECTION *new_section = NULL;
	INIFILE_KEY *cur_key = NULL;
	INIFILE_KEY *new_key = NULL;

	if (inifile == NULL || section == NULL || key == NULL || value == NULL)
	{
		INI_UNLOCK();
		return -1;
	}

	char sectionName[32];
	char keyName[32];
	char values[64];
	snprintf(sectionName, sizeof(sectionName), "%s", section);
	snprintf(keyName, sizeof(keyName), "%s", key);
	snprintf(values, sizeof(values), "%s", value);
	str_trimlr(sectionName);
	str_trimlr(keyName);
	str_trimlr(values);

	cur_section = inifile->sectionList;
	while (cur_section)
	{
		if (strcasecmp(sectionName, cur_section->name) == 0)
		{
			cur_key = cur_section->keyList;
			while (cur_key)
			{
				if (strcasecmp(keyName, cur_key->key) == 0)
				{
					snprintf(cur_key->value, sizeof(cur_key->value), "%s", values);
					INI_UNLOCK();
					return 0;
				}
				cur_key = cur_key->next;
			}
			new_key = (INIFILE_KEY *)malloc(sizeof(INIFILE_KEY));
			if (new_key == NULL)
			{
				INI_UNLOCK();
				return -1;
			}
			snprintf(new_key->key, sizeof(new_key->key), "%s", keyName);
			snprintf(new_key->value, sizeof(new_key->value), "%s", values);
			new_key->next = NULL;
			if (cur_section->keyList == NULL)
			{
				cur_section->keyList = new_key;
			}
			else
			{
				cur_key = cur_section->keyList;
				while (cur_key)
				{
					if (cur_key->next == NULL)
					{
						break;
					}
					cur_key = cur_key->next;
				}
				cur_key->next = new_key;
			}
			INI_UNLOCK();
			return 0;
		}
		cur_section = cur_section->next;
	}

	new_section = (INIFILE_SECTION *)malloc(sizeof(INIFILE_SECTION));
	if (new_section == NULL)
	{
		INI_UNLOCK();
		return -1;
	}

	new_key = (INIFILE_KEY *)malloc(sizeof(INIFILE_KEY));
	if (new_key == NULL)
	{
		free(new_section);
		INI_UNLOCK();
		return -1;
	}
	snprintf(new_key->key, sizeof(new_key->key), "%s", keyName);
	snprintf(new_key->value, sizeof(new_key->value), "%s", values);
	new_key->next = NULL;

	snprintf(new_section->name, sizeof(new_section->name), "%s", sectionName);
	new_section->keyList = new_key;
	new_section->next = NULL;

	if (inifile->sectionList == NULL)
	{
		inifile->sectionList = new_section;
	}
	else
	{
		cur_section = inifile->sectionList;
		while (cur_section)
		{
			if (cur_section->next == NULL)
			{
				break;
			}
			cur_section = cur_section->next;
		}
		cur_section->next = new_section;
	}

	INI_UNLOCK();
	return 0;
}

int inifile_delete(INIFILE *inifile, char *section, char *key)
{
	INI_LOCK();
	INIFILE_SECTION *cur_section = NULL;
	INIFILE_KEY *cur_key = NULL;
	INIFILE_KEY *pre_key = NULL;

	if (inifile == NULL || section == NULL || key == NULL)
	{
		INI_UNLOCK();
		return -1;
	}

	char sectionName[32];
	char keyName[32];
	snprintf(sectionName, sizeof(sectionName), "%s", section);
	snprintf(keyName, sizeof(keyName), "%s", key);
	str_trimlr(sectionName);
	str_trimlr(keyName);

	cur_section = inifile->sectionList;
	while (cur_section)
	{
		if (strcasecmp(sectionName, cur_section->name) == 0)
		{
			cur_key = cur_section->keyList;
			while (cur_key)
			{
				if (strcasecmp(keyName, cur_key->key) == 0)
				{
					if (pre_key == NULL)
					{
						cur_section->keyList = cur_key->next;
					}
					else
					{
						pre_key->next = cur_key->next;
					}
					free(cur_key);
					INI_UNLOCK();
					return 0;
				}
				pre_key = cur_key;
				cur_key = cur_key->next;
			}
			break;
		}
		cur_section = cur_section->next;
	}

	DBG_INFO("inifile delete fail: section[%s], key[%s]\n", sectionName, keyName);
	INI_UNLOCK();
	return -1;
}

int inifile_save(const char *fileName, INIFILE *inifile)
{
	INI_UNLOCK();
	FILE *fp = NULL;
	INIFILE_SECTION *cur_section;
	INIFILE_KEY *cur_key = NULL;

	if (fileName == NULL || inifile == NULL)
	{
		DBG_INFO("null pointer\n");
		INI_UNLOCK();
		return -1;
	}

	fp = fopen(fileName, "wb");
	if (fp == NULL)
	{
		INI_UNLOCK();
		return -1;
	}

	cur_section = inifile->sectionList;
	while (cur_section)
	{
		fprintf(fp, "[%s]\n", cur_section->name);
		cur_key = cur_section->keyList;
		while (cur_key)
		{
			fprintf(fp, "%s=%s\n", cur_key->key, cur_key->value);
			cur_key = cur_key->next;
		}
		cur_section = cur_section->next;
	}

	fclose(fp);
	INI_UNLOCK();
	return 0;
}

