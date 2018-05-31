
#include <errno.h>
#include <stdio.h>
#include <net/if.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>  
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "nl80211.h"
#include "iw.h"

#include "iwlist.h"
#include "iw_80211_scan.h"


#define NL_CB_KIND		(NL_CB_DEFAULT)


typedef int (*Func_CmdHandle)(struct nl80211_state *state,
			    struct nl_cb *cb,
			    struct nl_msg *msg,
			    int param1, int param2);


#ifndef CONFIG_LIBNL20
/* libnl 2.0 compatibility code */

static inline struct nl_handle *nl_socket_alloc(void)
{
	return nl_handle_alloc();
}

static inline void nl_socket_free(struct nl_sock *h)
{
	nl_handle_destroy(h);
}

static inline int __genl_ctrl_alloc_cache(struct nl_sock *h, struct nl_cache **cache)
{
	struct nl_cache *tmp = genl_ctrl_alloc_cache(h);
	if (!tmp)
		return -ENOMEM;
	*cache = tmp;
	return 0;
}
#define genl_ctrl_alloc_cache __genl_ctrl_alloc_cache
#endif /* CONFIG_LIBNL20 */

static int nl80211_init(struct nl80211_state *state)
{
	int err;

	state->nl_sock = nl_socket_alloc();
	if (!state->nl_sock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}

	if (genl_connect(state->nl_sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

	if (genl_ctrl_alloc_cache(state->nl_sock, &state->nl_cache)) {
		fprintf(stderr, "Failed to allocate generic netlink cache.\n");
		err = -ENOMEM;
		goto out_handle_destroy;
	}

	state->nl80211 = genl_ctrl_search_by_name(state->nl_cache, "nl80211");
	if (!state->nl80211) {
		fprintf(stderr, "nl80211 not found.\n");
		err = -ENOENT;
		goto out_cache_free;
	}

	return 0;

 out_cache_free:
	nl_cache_free(state->nl_cache);
 out_handle_destroy:
	nl_socket_free(state->nl_sock);
	return err;
}

static void nl80211_cleanup(struct nl80211_state *state)
{
	genl_family_put(state->nl80211);
	nl_cache_free(state->nl_cache);
	nl_socket_free(state->nl_sock);
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

static int iw_send_cmd(struct nl80211_state *state, int devidx, int cmd, int flags, Func_CmdHandle Cmd_Handle, int param1, int param2)
{
	struct nl_msg *msg = NULL;
	struct nl_cb *cb = NULL;
	struct nl_cb *s_cb = NULL;
	int err = 0;

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return 2;
	}

	cb = nl_cb_alloc(NL_CB_KIND);
	if (!cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		err = 2;
		goto out_free_msg;
	}

	s_cb = nl_cb_alloc(NL_CB_KIND);
	if (!s_cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		err = 2;
		goto out_free_msg;
	}

	genlmsg_put(msg, 0, 0, genl_family_get_id(state->nl80211), 0,
		    flags, cmd, 0);

	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);

	err = Cmd_Handle(state, cb, msg, param1, param2);
	if (err)
		goto out;

	nl_socket_set_cb(state->nl_sock, s_cb);

	err = nl_send_auto_complete(state->nl_sock, msg);
	if (err < 0)
		goto out;

	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

	while (err > 0)
		nl_recvmsgs(state->nl_sock, cb);
 out:
	nl_cb_put(cb);
 out_free_msg:
	nlmsg_free(msg);
	return err;
 nla_put_failure:
	fprintf(stderr, "building message failed\n");
	return 2;
}

static int iw_scan_trigger_handle(struct nl80211_state *state,
			    struct nl_cb *cb,
			    struct nl_msg *msg,
			    int param1, int param2)
{
	int err = -1;
	struct nl_msg *ssids = NULL;

	ssids = nlmsg_alloc();
	if (!ssids)
	{
		printf("failed to allocate ssids\n");
		return -ENOMEM;
	}

	NLA_PUT(ssids, 1, 0, "");
	nla_put_nested(msg, NL80211_ATTR_SCAN_SSIDS, ssids);

	err = 0;
nla_put_failure:
   nlmsg_free(ssids);
   return err;
}


struct iw_dump_param
{
	wifiap_t*	aplist;
	int*		cnt;
	int			searchedcnt;
};

#define MIN(a, b)		((a) < (b) ? (a) : (b))


/* Information Element IDs */
#define WLAN_EID_SSID 0
#define WLAN_EID_SUPP_RATES 1
#define WLAN_EID_FH_PARAMS 2
#define WLAN_EID_DS_PARAMS 3
#define WLAN_EID_CF_PARAMS 4
#define WLAN_EID_TIM 5
#define WLAN_EID_IBSS_PARAMS 6
#define WLAN_EID_COUNTRY 7
#define WLAN_EID_CHALLENGE 16
/* EIDs defined by IEEE 802.11h - START */
#define WLAN_EID_PWR_CONSTRAINT 32
#define WLAN_EID_PWR_CAPABILITY 33
#define WLAN_EID_TPC_REQUEST 34
#define WLAN_EID_TPC_REPORT 35
#define WLAN_EID_SUPPORTED_CHANNELS 36
#define WLAN_EID_CHANNEL_SWITCH 37
#define WLAN_EID_MEASURE_REQUEST 38
#define WLAN_EID_MEASURE_REPORT 39
#define WLAN_EID_QUITE 40
#define WLAN_EID_IBSS_DFS 41
/* EIDs defined by IEEE 802.11h - END */
#define WLAN_EID_ERP_INFO 42
#define WLAN_EID_HT_CAP 45
#define WLAN_EID_RSN 48
#define WLAN_EID_EXT_SUPP_RATES 50
#define WLAN_EID_MOBILITY_DOMAIN 54
#define WLAN_EID_FAST_BSS_TRANSITION 55
#define WLAN_EID_TIMEOUT_INTERVAL 56
#define WLAN_EID_RIC_DATA 57
#define WLAN_EID_HT_OPERATION 61
#define WLAN_EID_SECONDARY_CHANNEL_OFFSET 62
#define WLAN_EID_20_40_BSS_COEXISTENCE 72
#define WLAN_EID_20_40_BSS_INTOLERANT 73
#define WLAN_EID_OVERLAPPING_BSS_SCAN_PARAMS 74
#define WLAN_EID_MMIE 76
#define WLAN_EID_LINK_ID 101
#define WLAN_EID_ADV_PROTO 108
#define WLAN_EID_EXT_CAPAB 127
#define WLAN_EID_VENDOR_SPECIFIC 221

#define WPA_IE_VENDOR_TYPE 1
#define WPS_IE_VENDOR_TYPE 4
#define P2P_IE_VENDOR_TYPE 9

#define WPA_CIPHER_SUITE_NONE 0
#define WPA_CIPHER_SUITE_WEP40 1
#define WPA_CIPHER_SUITE_TKIP 2
#if 0
#define WPA_CIPHER_SUITE_WRAP 3
#endif
#define WPA_CIPHER_SUITE_CCMP 4
#define WPA_CIPHER_SUITE_WEP104 5



#define WLAN_CAPABILITY_ESS		(1<<0)
#define WLAN_CAPABILITY_IBSS		(1<<1)
#define WLAN_CAPABILITY_CF_POLLABLE	(1<<2)
#define WLAN_CAPABILITY_CF_POLL_REQUEST	(1<<3)
#define WLAN_CAPABILITY_PRIVACY		(1<<4)
#define WLAN_CAPABILITY_SHORT_PREAMBLE	(1<<5)
#define WLAN_CAPABILITY_PBCC		(1<<6)
#define WLAN_CAPABILITY_CHANNEL_AGILITY	(1<<7)
#define WLAN_CAPABILITY_SPECTRUM_MGMT	(1<<8)
#define WLAN_CAPABILITY_QOS		(1<<9)
#define WLAN_CAPABILITY_SHORT_SLOT_TIME	(1<<10)
#define WLAN_CAPABILITY_APSD		(1<<11)
#define WLAN_CAPABILITY_DSSS_OFDM	(1<<13)


static unsigned char ms_oui[3]		= { 0x00, 0x50, 0xf2 };
static unsigned char ieee80211_oui[3]	= { 0x00, 0x0f, 0xac };
static unsigned char wfa_oui[3]		= { 0x50, 0x6f, 0x9a };

// 解析加密方式，返回ENCODE_TYPE_AES
static int iw_cvt_cipher(const char* data, int defval)
{
	int type = defval;

	if (memcmp(data, ms_oui, 3) == 0 				// WPA oui
		|| memcmp(data, ieee80211_oui, 3) == 0) {	// RSN oui
		switch (data[3]) {
		case WPA_CIPHER_SUITE_NONE:
			break;
		case WPA_CIPHER_SUITE_WEP40:
		case WPA_CIPHER_SUITE_WEP104:
			type = ENCODE_TYPE_WEP;
			break;
		case WPA_CIPHER_SUITE_TKIP:
			type = ENCODE_TYPE_TKIP;
			break;
		case WPA_CIPHER_SUITE_CCMP:
			type = ENCODE_TYPE_AES;
			break;
		default:
			printf("%.02x-%.02x-%.02x:%d",
				data[0], data[1] ,data[2], data[3]);
			break;
		}
	} else
		printf("%.02x-%.02x-%.02x:%d",
			data[0], data[1] ,data[2], data[3]);

	return type;
}

static int iw_parse_wpa_ie(const char* data, int datalen, wifiap_t* ap)
{
	int count = 0;

	if (!ap)	return -1;

	if (!data || datalen < 2)
	{
		goto default_cipher;
	}

	data += 2;				// Version
	datalen -= 2;

	if (datalen < 4)		// Group cipher
	{
		goto default_cipher;
	}
	ap->iestat[1] = iw_cvt_cipher(data, ap->iestat[1]);
	data += 4;
	datalen -= 4;

	if (datalen < 2)		// Pairwise ciphers
	{
		goto default_cipher;
	}

	count = data[0] | (data[1] << 8);
	if (2 + (count * 4) > datalen)
	{
		goto default_cipher;
	}
	data += 2;
	datalen -= 2;

	// printf("%s, %.2x-%.2x-%.2x-%.2x\n", ap->name, data[0], data[1], data[2], data[3]);

	// 只解析一个
	ap->iestat[1] = iw_cvt_cipher(data, ap->iestat[1]);
	data += count * 4;
	datalen -= count * 4;

	// PSK、EAP等不做解析
	if (datalen < 2)		// Authentication suites
	{
	}

default_cipher:
	ap->iestat[1] = (ap->iestat[1] == ENCODE_TYPE_NONE) ? ENCODE_TYPE_AES : ap->iestat[1];

	return -1;
}

static int iw_cvt_rssi(int rssi)
{
	/* Normalize Rssi */
	if (rssi >= -50)
		return 100;
	else if (rssi >= -80) /* between -50 ~ -80dbm */
		return (unsigned char)(24 + ((rssi + 80) * 26) / 10);
	else if (rssi >= -90)   /* between -80 ~ -90dbm */
		return (unsigned char)((rssi + 90) * 26) / 10;
	else
		return 0;                             
}

static int iw_add_ssid(struct nl_msg *msg, void *arg)
{
	struct iw_dump_param *param = (struct iw_dump_param *)arg;
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *bss[NL80211_BSS_MAX + 1];
	static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
		[NL80211_BSS_TSF] = { .type = NLA_U64 },
		[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_BSS_BSSID] = { },
		[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
		[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
		[NL80211_BSS_INFORMATION_ELEMENTS] = { },
		[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
		[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
		[NL80211_BSS_STATUS] = { .type = NLA_U32 },
		[NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
		[NL80211_BSS_BEACON_IES] = { },
	};

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[NL80211_ATTR_BSS]) {
		fprintf(stderr, "bss info missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(bss, NL80211_BSS_MAX,
			     tb[NL80211_ATTR_BSS],
			     bss_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	if (!bss[NL80211_BSS_BSSID])
		return NL_SKIP;

	if (bss[NL80211_BSS_SIGNAL_MBM])
	{
		int rssi = (signed int)nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM]) / 100;
		param->aplist->quality = iw_cvt_rssi(rssi);
	}
	if (bss[NL80211_BSS_SIGNAL_UNSPEC])
	{
		param->aplist->quality = nla_get_u8(bss[NL80211_BSS_SIGNAL_UNSPEC]);
	}
	
	param->aplist->keystat = 0;
	param->aplist->iestat[0] = AUTH_TYPE_NONE;		// AUTH_TYPE_WPA
	param->aplist->iestat[1] = ENCODE_TYPE_NONE;	// ENCODE_TYPE_AES

	if (bss[NL80211_BSS_INFORMATION_ELEMENTS])
	{
		char* ie = nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
		int ielen = nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]);

		char* data = NULL;
		int	datalen = 0;	

		while (ielen >= 2 && ielen >= ie[1])
		{
			data = ie + 2;
			datalen = ie[1];

			// printf("iw_add_ssid: %d\n", ie[0]);

			switch (ie[0])
			{
			case WLAN_EID_SSID:
				snprintf(param->aplist->name, MIN(datalen + 1, sizeof(param->aplist->name)), "%s", data);
				// printf("ssid        : %s\n", param->aplist->name); 
				break;
			case WLAN_EID_RSN:
				param->aplist->keystat = 1;
				param->aplist->iestat[0] = AUTH_TYPE_WPA2;
				iw_parse_wpa_ie(data, datalen, param->aplist);
				break;
			case WLAN_EID_VENDOR_SPECIFIC:
				{
					if (datalen >= 4 && memcmp(data, ms_oui, 3) == 0)
					{
						// printf("IE_VENDOR: %d\n", data[3]);
						switch (data[3])
						{
						case WPA_IE_VENDOR_TYPE:
							param->aplist->keystat = 1;
							param->aplist->iestat[0] = AUTH_TYPE_WPA;
							data += 4;
							datalen -= 4;
							iw_parse_wpa_ie(data, datalen, param->aplist);
							break;
						default:
							break;
						}
					}
				}
				break;
			default:
				break;
			}
			ielen -= ie[1] + 2;
			ie += ie[1] + 2;
		}
	}

	if (bss[NL80211_BSS_CAPABILITY]) {
		unsigned short capa = nla_get_u16(bss[NL80211_BSS_CAPABILITY]);
		if ((param->aplist->keystat == 0) && (capa & WLAN_CAPABILITY_PRIVACY))
		{
			param->aplist->keystat = 1;
			param->aplist->iestat[0] = AUTH_TYPE_AUTO;
			param->aplist->iestat[1] = ENCODE_TYPE_WEP;
		}
	}

	++param->searchedcnt;
	++param->aplist;

	if (param->searchedcnt >= *param->cnt)
	{
		printf("iw_add_ssid, search result reaches limit: %d\n", *param->cnt);
		return NL_STOP;
	}

	return NL_SKIP;
}

static int iw_scan_dump_handle(struct nl80211_state *state,
			    struct nl_cb *cb,
			    struct nl_msg *msg,
			    int param1, int param2)
{
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, iw_add_ssid, (void*)param1);
	return 0;
}

int iw_80211_scan(const char* ifname, wifiap_t *aplist, int* maxCnt)
{
	struct nl80211_state state;
	int err = -ENOBUFS;
	int devidx = 0;
	const unsigned int cmds[] = {
		NL80211_CMD_NEW_SCAN_RESULTS,
		NL80211_CMD_SCAN_ABORTED,
	};
	static struct iw_dump_param param;

	if (!aplist || !maxCnt || *maxCnt <= 0)
	{
		printf("iw_scan, invalid arg!\n");
		return -1;
	}

	devidx = if_nametoindex(ifname);
	if (devidx == 0)
	{
		printf("if_nametoindex failed with %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	if (nl80211_init(&state) != 0)
	{
		return -ENOMEM;
	}

	err = iw_send_cmd(&state, devidx, NL80211_CMD_TRIGGER_SCAN, 0, iw_scan_trigger_handle, 0, 0);

	if (listen_events(&state, ARRAY_SIZE(cmds), cmds) == NL80211_CMD_SCAN_ABORTED)
	{
		printf("scan aborted!\n");
		return 0;
	}

	param.aplist = aplist;
	param.cnt = maxCnt;
	param.searchedcnt = 0;

	err = iw_send_cmd(&state, devidx, NL80211_CMD_GET_SCAN, NLM_F_DUMP, iw_scan_dump_handle, (int)&param, 0);

	nl80211_cleanup(&state);

	*param.cnt = param.searchedcnt;

	return err;
	
}
