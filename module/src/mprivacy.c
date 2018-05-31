

#include "jv_common.h"
#include "msnapshot.h"
#include "mprivacy.h"
#include "mstream.h"
#include "jv_privacy.h"
#include "mlog.h"
#include <msensor.h>

typedef struct
{
	REGION region[MAX_STREAM];
	BOOL started[MAX_STREAM];
}privacy_status_t;

static privacy_status_t privacystatus;

#if (defined PLATFORM_hi3516D || defined PLATFORM_hi3516CV200)
extern BOOL VI_CROP_ENABLE;
extern int VI_CROP_X;
extern int VI_CROP_Y;
extern int VI_CROP_W;
extern int VI_CROP_H;
#endif

int mprivacy_init(void)
{
	memset(&privacystatus, 0, sizeof(privacystatus));

	jv_privacy_init();
	return 0;
}

int mprivacy_deinit(void)
{
	Printf("mprivacy_deinit start privacystatus.started[0]:%d\n", privacystatus.started[0]);
	if (privacystatus.started[0])
	{
		privacystatus.region[0].bEnable = FALSE;
		mprivacy_flush(0);
	}
	jv_privacy_deinit();
	Printf("mprivacy_deinit over\n");
	return 0;
}

int mprivacy_stop(int channelid)
{
	return jv_privacy_stop(channelid);
}

int mprivacy_start(int channelid)
{
	return jv_privacy_start(channelid);
}

int mprivacy_get_param(int channelid, REGION *region)
{
	jv_assert(channelid >= 0 && channelid < HWINFO_STREAM_CNT, return JVERR_BADPARAM);
	jv_assert(region != NULL, return JVERR_BADPARAM);

	memcpy(region, &privacystatus.region[channelid], sizeof(REGION));
	return 0;
}

int mprivacy_set_param(int channelid, REGION *region)
{
	jvstream_ability_t ability;
	int i;
	jv_assert(channelid >= 0 && channelid < HWINFO_STREAM_CNT, return JVERR_BADPARAM);
	jv_assert(region != NULL, return JVERR_BADPARAM);

	if (privacystatus.region[channelid].bEnable != region->bEnable)
	{
		if (region->bEnable)
			mlog_write("Privacy Enabled");
		else
			mlog_write("Privacy Disabled");
	}
	for (i=0;i<MAX_PYRGN_NUM;i++)
	{
		if (memcmp(&privacystatus.region[channelid].stRect[i], &region->stRect[i], sizeof(RECT)) != 0)
			mlog_write("Privacy Region Modified: [%d,%d,%d,%d]", region->stRect[i].x, region->stRect[i].y, region->stRect[i].w, region->stRect[i].h);
	}

	privacystatus.region[channelid].bEnable = region->bEnable;
	
	jv_stream_get_ability(channelid, &ability);

	for (i=0;i<MAX_PYRGN_NUM;i++)
	{
		privacystatus.region[channelid].stRect[i].x = VALIDVALUE (region->stRect[i].x, 0, ability.inputRes.width);
		privacystatus.region[channelid].stRect[i].y = VALIDVALUE (region->stRect[i].y, 0, ability.inputRes.height);
		privacystatus.region[channelid].stRect[i].w = VALIDVALUE (region->stRect[i].w, 0, ability.inputRes.width - privacystatus.region[channelid].stRect[i].x);
		privacystatus.region[channelid].stRect[i].h = VALIDVALUE (region->stRect[i].h, 0, ability.inputRes.height - privacystatus.region[channelid].stRect[i].y);
	}

	return 0;
}

int mprivacy_flush(int channelid)
{
	int i = 0;
	jv_privacy_attr_t attr;
	memset(&attr, 0, sizeof(jv_privacy_attr_t));
	jv_assert(channelid >= 0 && channelid < HWINFO_STREAM_CNT, return JVERR_BADPARAM);

	printf("mprivacy_flush chn=%d, enable=%d\n", channelid, privacystatus.region[channelid].bEnable);
	if (privacystatus.region[channelid].bEnable)
	{
		attr.cnt = 0;
		attr.bEnable = TRUE;
		for (i=0;i<MAX_PYRGN_NUM;i++)
		{
			if ((privacystatus.region[channelid].stRect[i]).w > 0 && (privacystatus.region[channelid].stRect[i]).h > 0)
			{
				attr.rect[attr.cnt++] = privacystatus.region[channelid].stRect[i];
			}
		}
		JVRotate_e rotate = msensor_get_rotate();
		unsigned int viw, vih;
		jv_stream_get_vi_resolution(0, &viw, &vih);
#if (defined PLATFORM_hi3516D || defined PLATFORM_hi3516CV200)
		if(VI_CROP_ENABLE)
		{
			viw = VI_CROP_W;
			vih = VI_CROP_H;
		}
#endif
		if (rotate != JVSENSOR_ROTATE_NONE)
		{
			for (i=0;i<attr.cnt;i++)
				msensor_rotate_calc(rotate, viw, vih, &attr.rect[i]);
		}
		//即使为0，也得通知底层
		//if (attr.cnt > 0)
		{
			privacystatus.started[channelid] = 1;
			jv_privacy_set_attr(channelid,&attr);
			jv_privacy_start(channelid);
		}
	}
	else if (privacystatus.started[channelid])
	{
		privacystatus.started[channelid] = 0;
		jv_privacy_stop(channelid);
	}
	Printf("mprivacy_flush succeed!\n");
	return 0;
}

