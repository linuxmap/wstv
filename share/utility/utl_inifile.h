#ifndef _UTL_INIFILE_H_
#define _UTL_INIFILE_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct INIFILE_KEY
{
	char key[32];
	char value[64];
	struct INIFILE_KEY *next;
}INIFILE_KEY;

typedef struct INIFILE_SECTION
{
	char name[32];
	INIFILE_KEY *keyList;
	struct INIFILE_SECTION *next;
}INIFILE_SECTION;

typedef struct INIFILE
{
	INIFILE_SECTION *sectionList;
}INIFILE;

int inifile_init(const char *fileName, INIFILE *inifile);
void inifile_free(INIFILE *inifile);
int inifile_get(INIFILE *inifile, char *section, char *key, char *value, int size);
int inifile_put(INIFILE *inifile, char *section, char *key, char *value);
int inifile_delete(INIFILE *inifile, char *section, char *key);
int inifile_save(const char *fileName, INIFILE *inifile);

#ifdef __cplusplus
}; //end of extern "C" {
#endif

#endif //end of _UTL_INIFILE_H_