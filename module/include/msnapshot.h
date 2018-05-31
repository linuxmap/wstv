#ifndef _MSNAPSHOT_H_
#define _MSNAPSHOT_H_
#include <jv_snapshot.h>

#define msnapshot_get jv_snapshot_get

/**
 *@brief snap shot
 *@param channelid channel id
 *@param fname file name to store the capture
 *@retval 0 if success
 *@retval <0 if failed.
 */
int msnapshot_get_file(int channelid, char *fname);

int msnapshot_get_file_ex(int channelid, char *fname, int width, int height);

int msnapshot_get_data(int channelid, unsigned char *pData ,unsigned int len, stSnapSize *snap);
int msnapshot_get_shmdata(int channelid);

#endif
