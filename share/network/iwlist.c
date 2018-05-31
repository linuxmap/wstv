/*
 *	Wireless Tools
 *
 *		Jean II - HPLB '99 - HPL 99->07
 *
 * This tool can access various piece of information on the card
 * not part of iwconfig...
 * You need to link this code against "iwlist.c" and "-lm".
 *
 * This file is released under the GPL license.
 *     Copyright (c) 1997-2007 Jean Tourrilhes <jt@hpl.hp.com>
 */

#include "iwlib.h"		/* Header */
#include <sys/time.h>
//#include"iwlist.h"
#include<stdlib.h>
#include<stdio.h>
#include <utl_system.h>
#include "iwlist.h"
/****************************** TYPES ******************************/

IPC_NET ipc_net;

/*
 * Scan state and meta-information, used to decode events...
 */

#ifndef Printf
#define Printf(fmt...)  \
do{\
	if(0){	\
		printf("[%s]:%d ", __FILE__, __LINE__);\
		printf(fmt);} \
} while(0)
#endif

 
#define  MAX_AP 100
int  count=0;
typedef struct iwscan_state
{
  /* State */
  int			ap_num;		/* Access Point number 1->N */
  int			val_index;	/* Value in table 0->(N-1) */
} iwscan_state;

/**************************** CONSTANTS ****************************/

#define IW_SCAN_HACK		0x8000

#define IW_EXTKEY_SIZE	(sizeof(struct iw_encode_ext) + IW_ENCODING_TOKEN_MAX)

/* ------------------------ WPA CAPA NAMES ------------------------ */
/*
 * This is the user readable name of a bunch of WPA constants in wireless.h
 * Maybe this should go in iwlib.c ?
 */

#ifndef WE_ESSENTIAL
#define IW_ARRAY_LEN(x) (sizeof(x)/sizeof((x)[0]))

//static const struct iwmask_name iw_enc_mode_name[] = {
//  { IW_ENCODE_RESTRICTED,	"restricted" },
//  { IW_ENCODE_OPEN,		"open" },
//};
//#define	IW_ENC_MODE_NUM		IW_ARRAY_LEN(iw_enc_mode_name)

#define	IW_AUTH_CAPA_NUM	IW_ARRAY_LEN(iw_auth_capa_name)


#define	IW_AUTH_CYPHER_NUM	IW_ARRAY_LEN(iw_auth_cypher_name)


#define	IW_WPA_VER_NUM		IW_ARRAY_LEN(iw_wpa_ver_name)


#define	IW_AUTH_KEY_MGMT_NUM	IW_ARRAY_LEN(iw_auth_key_mgmt_name)


#define	IW_AUTH_ALG_NUM		IW_ARRAY_LEN(iw_auth_alg_name)


#define	IW_AUTH_SETTINGS_NUM		IW_ARRAY_LEN(iw_auth_settings)

/* Values for the IW_ENCODE_ALG_* returned by SIOCSIWENCODEEXT */

#define	IW_ENCODE_ALG_NUM		IW_ARRAY_LEN(iw_encode_alg_name)

#ifndef IW_IE_CIPHER_NONE
/* Cypher values in GENIE (pairwise and group) */
#define IW_IE_CIPHER_NONE	0
#define IW_IE_CIPHER_WEP40	1
#define IW_IE_CIPHER_TKIP	2
#define IW_IE_CIPHER_WRAP	3
#define IW_IE_CIPHER_CCMP	4
#define IW_IE_CIPHER_WEP104	5
/* Key management in GENIE */
#define IW_IE_KEY_MGMT_NONE	0
#define IW_IE_KEY_MGMT_802_1X	1
#define IW_IE_KEY_MGMT_PSK	2
#endif	/* IW_IE_CIPHER_NONE */

/* Values for the IW_IE_CIPHER_* in GENIE */
static const char *	iw_ie_cypher_name[] = {
	"none",
	"WEP-40",
	"TKIP",
	"WRAP",
	"CCMP",
	"WEP-104",
};
#define	IW_IE_CYPHER_NUM	IW_ARRAY_LEN(iw_ie_cypher_name)

/* Values for the IW_IE_KEY_MGMT_* in GENIE */
static const char *	iw_ie_key_mgmt_name[] = {
	"none",
	"802.1x",
	"PSK",
};
#define	IW_IE_KEY_MGMT_NUM	IW_ARRAY_LEN(iw_ie_key_mgmt_name)

#endif	/* WE_ESSENTIAL */

/************************* WPA SUBROUTINES *************************/


/*
 * Print the name corresponding to a value, with overflow check.
 */
static void
iw_print_value_name(unsigned int		value,
		    const char *		names[],
		    const unsigned int		num_names)
{
  if(value >= num_names)
    Printf(" unknown (%d)", value);
  else
    Printf(" %s", names[value]);
}

static void 
iw_print_ie_unknown(unsigned char *	iebuf,
		    int			buflen)
{
  int	ielen = iebuf[1] + 2;
  int	i;

  if(ielen > buflen)
    ielen = buflen;

  Printf("Unknown: ");
  for(i = 0; i < ielen; i++)
    Printf("%02X", iebuf[i]);
  Printf("\n");
}

static inline void 
iw_print_ie_wpa(unsigned char *	iebuf,
		int		buflen,char* index)
{
  int			ielen = iebuf[1] + 2;
  int			offset = 2;	/* Skip the IE id, and the length. */
  unsigned char		wpa1_oui[3] = {0x00, 0x50, 0xf2};
  unsigned char		wpa2_oui[3] = {0x00, 0x0f, 0xac};
  unsigned char *	wpa_oui;
  int			i;
  uint16_t		ver = 0;
  uint16_t		cnt = 0;

  if(ielen > buflen)
    ielen = buflen;

#ifdef DEBUG
  /* Debugging code. In theory useless, because it's debugged ;-) */
  printf("IE raw value %d [%02X", buflen, iebuf[0]);
  for(i = 1; i < buflen; i++)
    printf(":%02X", iebuf[i]);
  printf("]\n");
#endif

  switch(iebuf[0])
    {
    case 0x30:		/* WPA2 */
      /* Check if we have enough data */
      if(ielen < 4)
	{
	  iw_print_ie_unknown(iebuf, buflen);
 	  return;
	}

      wpa_oui = wpa2_oui;
      break;

    case 0xdd:		/* WPA or else */
      wpa_oui = wpa1_oui;
 
      /* Not all IEs that start with 0xdd are WPA. 
       * So check that the OUI is valid. Note : offset==2 */
      if((ielen < 8)
	 || (memcmp(&iebuf[offset], wpa_oui, 3) != 0)
	 || (iebuf[offset + 3] != 0x01))
 	{
	  iw_print_ie_unknown(iebuf, buflen);
 	  return;
 	}

      /* Skip the OUI type */
      offset += 4;
      break;

    default:
      return;
    }
  
  /* Pick version number (little endian) */
  ver = iebuf[offset] | (iebuf[offset + 1] << 8);
  offset += 2;

  if(iebuf[0] == 0xdd)
    Printf("WPA Version %d\n", ver);
  if(iebuf[0] == 0x30)
    Printf("IEEE 802.11i/WPA2 Version %d\n", ver);

  /* From here, everything is technically optional. */

  /* Check if we are done */
  if(ielen < (offset + 4))
    {
      /* We have a short IE.  So we should assume TKIP/TKIP. */
      Printf("                        Group Cipher : TKIP\n");
      Printf("                        Pairwise Cipher : TKIP\n");
      return;
    }
 
  /* Next we have our group cipher. */
  if(memcmp(&iebuf[offset], wpa_oui, 3) != 0)
    {
      Printf("                        Group Cipher : Proprietary\n");
    }
  else
    {
      Printf("                        Group Cipher :");
      iw_print_value_name(iebuf[offset+3],
			  iw_ie_cypher_name, IW_IE_CYPHER_NUM);
	  *index = iebuf[offset+3];
    }
  offset += 4;
	//printf("\n");
  /* Check if we are done */
  if(ielen < (offset + 2))
    {
      /* We don't have a pairwise cipher, or auth method. Assume TKIP. */
      Printf("                        Pairwise Ciphers : TKIP\n");
      return;
    }

  /* Otherwise, we have some number of pairwise ciphers. */
  cnt = iebuf[offset] | (iebuf[offset + 1] << 8);
  offset += 2;
  Printf("                        Pairwise Ciphers (%d) :", cnt);

  if(ielen < (offset + 4*cnt))
    return;

  for(i = 0; i < cnt; i++)
    {
      if(memcmp(&iebuf[offset], wpa_oui, 3) != 0)
 	{
 	  Printf(" Proprietary");
 	}
      else
	{
	  iw_print_value_name(iebuf[offset+3],
			      iw_ie_cypher_name, IW_IE_CYPHER_NUM);
 	}
      offset+=4;
    }
  Printf("\n");
 
  /* Check if we are done */
  if(ielen < (offset + 2))
    return;

  /* Now, we have authentication suites. */
  cnt = iebuf[offset] | (iebuf[offset + 1] << 8);
  offset += 2;
 // printf("                        Authentication Suites (%d) :", cnt);

  if(ielen < (offset + 4*cnt))
    return;

  for(i = 0; i < cnt; i++)
    {
      if(memcmp(&iebuf[offset], wpa_oui, 3) != 0)
 	{
 	  printf(" Proprietary");
 	}
      else
	{
	  iw_print_value_name(iebuf[offset+3],
			      iw_ie_key_mgmt_name, IW_IE_KEY_MGMT_NUM);
 	}
       offset+=4;
     }
  Printf("\n");
 
  /* Check if we are done */
  if(ielen < (offset + 1))
    return;

  /* Otherwise, we have capabilities bytes.
   * For now, we only care about preauth which is in bit position 1 of the
   * first byte.  (But, preauth with WPA version 1 isn't supposed to be 
   * allowed.) 8-) */
  if(iebuf[offset] & 0x01)
    {
      Printf("                       Preauthentication Supported\n");
    }
}

static inline int
iw_print_gen_ie(unsigned char *	buffer,
		int		buflen,
		char * ie)
{
  int offset = 0;
  int			ielen = buffer[1] + 2;
  unsigned char		wpa1_oui[3] = {0x00, 0x50, 0xf2};
  unsigned char		wpa2_oui[3] = {0x00, 0x0f, 0xac};
  unsigned char *	wpa_oui;
  /* Loop on each IE, each IE is minimum 2 bytes */
  while(offset <= (buflen - 2))
    {
    //  printf("                    IE: ");
      /* Check IE type */
      switch(buffer[offset])
	{
	case 0xdd:	/* WPA1 (and other) */
		wpa_oui = wpa1_oui;
		if((ielen < 8)
			 || (memcmp(&buffer[offset+2], wpa_oui, 3) != 0)
			 || (buffer[offset + 5] != 0x01))
		 	{
			strcpy(ie,"unknow");
		 	}
		else
			strcpy(ie,"wpa");
	break;
	case 0x30:	/* WPA2 */
	  //iw_print_ie_wpa(buffer + offset, buflen);
	  strcpy(ie,"wpa2");
	  break;
	default:
	strcpy(ie,"free");
	  //iw_print_ie_unknown(buffer + offset, buflen);
	  break;
	}
      /* Skip over this IE to the next one in the list. */
      offset += buffer[offset+1] + 2;
    }
    return 0;
}


 
 
 
static inline int
print_scanning_token(struct stream_descr *	stream,	/* Stream of events */
		     struct iw_event *		event,	/* Extracted token */
		     struct iwscan_state *	state,
		     struct iw_range *	iw_range,	/* Range info */
		     int		has_range)
{
  char		buffer[128];	/* Temporary buffer */
 //char src[4];
 char ptr[8];
 //static int count=0;
  /* Now, let's decode the event */
  switch(event->cmd)
    {
    
    case SIOCGIWESSID:
      {
	char essid[IW_ESSID_MAX_SIZE+1];
	memset(essid, '\0', sizeof(essid));
	if((event->u.essid.pointer) && (event->u.essid.length))
	  memcpy(essid, event->u.essid.pointer, event->u.essid.length);
	if(event->u.essid.flags)
	  {
	    /* Does it have an ESSID index ? */
	    if((event->u.essid.flags & IW_ENCODE_INDEX) > 1)
	      Printf("                    ESSID:\"%s\" [%d]\n", essid,
		     (event->u.essid.flags & IW_ENCODE_INDEX));
	    else
	    {
	      Printf("                    ESSID:\"%s\"\n", essid);
	      
	      if(strlen(essid) != 0)
	      {
	    	  memcpy(ipc_net.wifi.wifiap[count].name,essid,32);
	    	  count++;
	      }
	  //    printf(ipc_net.wifi.wifiap[0].name);
	     } 
	    //这个地方，如果搜到ap数量多于MAX_AP则用覆盖掉原来的
	    //也就是说最后只显示比maxap大的几个，并没有按照信号强弱排序，我觉得有问题，
	    //由于测试的时候ap比较多，我暂时只把MAX_AP由10改成了20，并没有按照排序显示前10个;
	    //要改回去需要该两个宏定义，还有一个MAX_WIFI_CNT,lk20131206
	     if(count >= MAX_AP)
	     {
	     	count=0;
	     }

	  }
	else
	  Printf("                    ESSID:off/any/hidden\n");
      }
      break;
    case SIOCGIWENCODE:
      {
	unsigned char	key[IW_ENCODING_TOKEN_MAX];
	if(event->u.data.pointer)
	  memcpy(key, event->u.data.pointer, event->u.data.length);
	else
	  event->u.data.flags |= IW_ENCODE_NOKEY;
//	printf("                    Encryption key:%s\n",key);
	if(event->u.data.flags & IW_ENCODE_DISABLED)
	{
	  //printf("lk test no encode\n");
	  //strcpy(ipc_net.wifi.wifiap[count-1].keystat,"off");
		ipc_net.wifi.wifiap[count-1].keystat=0;
		ipc_net.wifi.wifiap[count-1].iestat[0] = 5;
		ipc_net.wifi.wifiap[count-1].iestat[1] = 0;
	  }
	else
	  {
	    /* Display the key */
	    iw_print_key(buffer, sizeof(buffer), key, event->u.data.length,
			 event->u.data.flags);
	    //不加这一句会造成连续搜索时把有密码的搞成没密码的。lk20131206
	    ipc_net.wifi.wifiap[count-1].keystat=1;
	    ipc_net.wifi.wifiap[count-1].iestat[0] = 1;
	    ipc_net.wifi.wifiap[count-1].iestat[1] = 1;
	    //printf("lk test key info:%s", buffer);
	   // strcpy(ipc_net.wifi.wifiap[count-1].keystat,buffer);
	  }
	//printf("lk test ap count :%d\n",count);
      }
      break;
    case SIOCGIWMODUL:
      {
	unsigned int	modul = event->u.param.value;
	int		i;
	int		n = 0;
	//printf("                    Modulations :");
	for(i = 0; i < IW_SIZE_MODUL_LIST; i++)
	  {
	    if((modul & iw_modul_list[i].mask) == iw_modul_list[i].mask)
	      {
		if((n++ % 8) == 7)
		  Printf("\n                        ");
		else
		  Printf(" ; ");
		Printf("%s", iw_modul_list[i].cmd);
	      }
	  }
	Printf("\n");
      }
      break;
    case IWEVQUAL:
      /*iw_print_stats(buffer, sizeof(buffer),
		     &event->u.qual, iw_range, has_range);*/
		     
      //printf("                    %s\n", buffer);
     
      //sprintf(src,"%d",event->u.qual.qual);
      //printf("\n\n\n");
      //printf(src);
    //  printf("\n\n\n");
      ipc_net.wifi.wifiap[count-1].quality=event->u.qual.qual;
	  ipc_net.wifi.wifiap[count-1].iestat[4] = event->u.qual.level;
      break;
#ifndef WE_ESSENTIAL
    case IWEVGENIE:
      /* Informations Elements are complex, let's do only some of them */
      iw_print_gen_ie(event->u.data.pointer, event->u.data.length,ptr);
      //printf("%d  ",count-1);
     // printf(ptr);
	  Printf("\n");
      //strcpy(ipc_net.wifi.wifiap[count-1].iestat,ptr);
      
		//应该还有其他的认证方式，但是在这里却获取不到(没有走这个case)，不知道为什么
	  if(!strcmp("wep",ptr))
	  {
		  ipc_net.wifi.wifiap[count-1].iestat[0] = 2;
	  }
	  else if(!strcmp("wpa",ptr))
	  {
	      ipc_net.wifi.wifiap[count-1].iestat[0] = 3;
	  }
	  else if(!strcmp("wpa2",ptr))
	  {
	      ipc_net.wifi.wifiap[count-1].iestat[0] = 4;
	  }
	  
	  char index = 0;
	  iw_print_ie_wpa(event->u.data.pointer, event->u.data.length,&index);
	  //将CCMP类型作为AES类型的话，这样可以跟家用设别加密类型对应起来
//	  printf(".............................................%d\n",index);
	  index = index > 3 ? 3 : index;
	  if(index>0)
		  ipc_net.wifi.wifiap[count-1].iestat[1] = index;
      break;
#endif	/* WE_ESSENTIAL */
    case IWEVCUSTOM:
      {
	char custom[IW_CUSTOM_MAX+1];
	if((event->u.data.pointer) && (event->u.data.length))
	  memcpy(custom, event->u.data.pointer, event->u.data.length);
	custom[event->u.data.length] = '\0';
//	printf("                    Extra:%s\n", custom);//打印搜索结果
      }
      break;
    default:
     break;
   }	/* switch(event->cmd) */
  // printf("\n******%d\n",count);
   return count;
}

/*------------------------------------------------------------------*/
/*
 * Perform a scanning on one device
 */
static int
print_scanning_info(int * ap_count,		//ap个数输出
		    char *	ifname,				//网卡名成如：eth0，wlan0
		    char *	args[],		/* Command line args */
		    int		count)		/* Args count */
{
  struct iwreq		wrq;
  struct iw_scan_req    scanopt;		/* Options for 'set' */
  int			scanflags = 0;		/* Flags for scan */
  unsigned char *	buffer = NULL;		/* Results */
  int			buflen = IW_SCAN_MAX_DATA; /* Min for compat WE<17 */
  struct iw_range	range;
  int			has_range;
  struct timeval	tv;				/* Select timeout */
  int			timeout = 15000000;		/* 15s */
  int skfd;
  //********************************lk debug*********************************************
  int lkret;

 if((skfd = iw_sockets_open()) < 0)
    {
      perror("socket");
      return -1;
    }
 //******************************lk debug**************************************************
 //printf("*********scan info skfd:%d **********\n",skfd);


  /* Avoid "Unused parameter" warning */
  args = args; count = count;
  /* Debugging stuff */
  if((IW_EV_LCP_PK2_LEN != IW_EV_LCP_PK_LEN) || (IW_EV_POINT_PK2_LEN != IW_EV_POINT_PK_LEN))
    {
      fprintf(stderr, "*** Please report to jt@hpl.hp.com your platform details\n");
      fprintf(stderr, "*** and the following line :\n");
      fprintf(stderr, "*** IW_EV_LCP_PK2_LEN = %zu ; IW_EV_POINT_PK2_LEN = %zu\n\n",
	      IW_EV_LCP_PK2_LEN, IW_EV_POINT_PK2_LEN);
    }

  /* Get range stuff */
  has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);
  //******************************lk debug**************************************************
  //printf("*********scan info has_range:%d **********\n",has_range);

  /* Check if the interface could support scanning. */
  if((!has_range) || (range.we_version_compiled < 14))
    {
      fprintf(stderr, "%-8.16s  Interface doesn't support scanning.\n\n",
	      ifname);
      return(-1);
    }

  /* Init timeout value -> 250ms between set and first get */
  tv.tv_sec = 0;
  tv.tv_usec = 250000;

  /* Clean up set args */
  memset(&scanopt, 0, sizeof(scanopt));

  /* Parse command line arguments and extract options.
   * Note : when we have enough options, we should use the parser
   * from iwconfig... */
  while(count > 0)
    {
      /* One arg is consumed (the option name) */
      count--;

      /*
       * Check for Active Scan (scan with specific essid)
       */
      if(!strncmp(args[0], "essid", 5))
	{
	  if(count < 1)
	    {
	      fprintf(stderr, "Too few arguments for scanning option [%s]\n",
		      args[0]);
	      return(-1);
	    }
	  args++;
	  count--;

	  /* Store the ESSID in the scan options */
	  scanopt.essid_len = strlen(args[0]);
	  memcpy(scanopt.essid, args[0], scanopt.essid_len);
	  /* Initialise BSSID as needed */
	  if(scanopt.bssid.sa_family == 0)
	    {
	      scanopt.bssid.sa_family = ARPHRD_ETHER;
	      memset(scanopt.bssid.sa_data, 0xff, ETH_ALEN);
	    }
	  /* Scan only this ESSID */
	  scanflags |= IW_SCAN_THIS_ESSID;
	}
      else
	/* Check for last scan result (do not trigger scan) */
	if(!strncmp(args[0], "last", 4))
	  {
	    /* Hack */
	    scanflags |= IW_SCAN_HACK;
	  }
	else
	  {
	    fprintf(stderr, "Invalid scanning option [%s]\n", args[0]);
	    return(-1);
	  }

      /* Next arg */
      args++;
    }

  /* Check if we have scan options */
  if(scanflags)
    {
      wrq.u.data.pointer = (caddr_t) &scanopt;
      wrq.u.data.length = sizeof(scanopt);
      wrq.u.data.flags = scanflags;
    }
  else
    {
      wrq.u.data.pointer = NULL;
      wrq.u.data.flags = 0;
      wrq.u.data.length = 0;
    }

  /* If only 'last' was specified on command line, don't trigger a scan */
  if(scanflags == IW_SCAN_HACK)
    {
      /* Skip waiting */
      tv.tv_usec = 0;
    }
  else
    {
      /* Initiate Scanning */
	  	 	   lkret = iw_set_ext(skfd, ifname, SIOCSIWSCAN, &wrq);
		//******************************lk debug**************************************************
		//printf("*********scan info iw_set_ext:%d **********\n",lkret);
      if(lkret < 0)
	{
	  if((errno != EPERM) || (scanflags != 0))
	    {
	      fprintf(stderr, "%-8.16s  Interface doesn't support scanning : %s\n\n",
		      ifname, strerror(errno));
	      return(-1);
	    }
	  /* If we don't have the permission to initiate the scan, we may
	   * still have permission to read left-over results.
	   * But, don't wait !!! */
#if 0
	  /* Not cool, it display for non wireless interfaces... */
	  fprintf(stderr, "%-8.16s  (Could not trigger scanning, just reading left-over results)\n", ifname);
#endif
	  tv.tv_usec = 0;
	}
    }
  timeout -= tv.tv_usec;

  /* Forever */
  while(1)
    {
      fd_set		rfds;		/* File descriptors for select */
      int		last_fd;	/* Last fd */
      int		ret;

      /* Guess what ? We must re-generate rfds each time */
      FD_ZERO(&rfds);
      last_fd = -1;

      /* In here, add the rtnetlink fd in the list */

      /* Wait until something happens */
      ret = select(last_fd + 1, &rfds, NULL, NULL, &tv);

      /* Check if there was an error */
      if(ret < 0)
	{
	  if(errno == EAGAIN || errno == EINTR)
	    continue;
	  fprintf(stderr, "Unhandled signal - exiting...\n");
	  return(-1);
	}

      /* Check if there was a timeout */
      if(ret == 0)
	{
	  unsigned char *	newbuf;

	realloc:
	  /* (Re)allocate the buffer - realloc(NULL, len) == malloc(len) */
	  newbuf = realloc(buffer, buflen);
	  if(newbuf == NULL)
	    {
	      if(buffer)
		free(buffer);
	      fprintf(stderr, "%s: Allocation failed\n", __FUNCTION__);
	      return(-1);
	    }
	  buffer = newbuf;

	  /* Try to read the results */
	  wrq.u.data.pointer = buffer;
	  wrq.u.data.flags = 0;
	  wrq.u.data.length = buflen;
	  lkret = iw_get_ext(skfd, ifname, SIOCGIWSCAN, &wrq);
//			//******************************lk debug**************************************************
//			printf("*********scan info iw_get_ext:%d **********\n",
//					lkret);
	  if(lkret < 0)
	    {
	      /* Check if buffer was too small (WE-17 only) */
	      if((errno == E2BIG) && (range.we_version_compiled > 16))
		{
					//******************************lk debug**************************************************
		//printf("*********scan info we_version_compiled:%d **********\n",range.we_version_compiled);
		  /* Some driver may return very large scan results, either
		   * because there are many cells, or because they have many
		   * large elements in cells (like IWEVCUSTOM). Most will
		   * only need the regular sized buffer. We now use a dynamic
		   * allocation of the buffer to satisfy everybody. Of course,
		   * as we don't know in advance the size of the array, we try
		   * various increasing sizes. Jean II */

		  /* Check if the driver gave us any hints. */
//
		  if(wrq.u.data.length > buflen)
		    buflen = wrq.u.data.length;
		  else
		    buflen *= 2;

		  /* Try again */
		  goto realloc;
		}

	      /* Check if results not available yet */
	      if(errno == EAGAIN)
		{
		  /* Restart timer for only 100ms*/
//					//******************************lk debug**************************************************
//					printf("************************lk test errno 737***************\n");
		  tv.tv_sec = 0;
		  tv.tv_usec = 100000;
		  timeout -= tv.tv_usec;
		  if(timeout > 0)
		    continue;	/* Try again later */
		}

	      /* Bad error */
	      free(buffer);
	      fprintf(stderr, "%-8.16s  Failed to read scan data : %s\n\n",
		      ifname, strerror(errno));
	      return(-2);
	    }
	  else
	    /* We have the results, go to process them */
	    break;
	}

      /* In here, check if event and event type
       * if scan event, read results. All errors bad & no reset timeout */
    }
//  ******************************lk debug**************************************************
	//printf("*********scan info length:%d **********\n",wrq.u.data.length);
  iw_sockets_close(skfd);
  if(wrq.u.data.length)
    {
      struct iw_event		iwe;
      struct stream_descr	stream;
      struct iwscan_state	state = { .ap_num = 1, .val_index = 0 };
      int			ret;
      
#ifdef DEBUG
      /* Debugging code. In theory useless, because it's debugged ;-) */
      int	i;
      printf("Scan result %d [%02X", wrq.u.data.length, buffer[0]);
      for(i = 1; i < wrq.u.data.length; i++)
	printf(":%02X", buffer[i]);
      printf("]\n");
#endif
      //printf("%-8.16s  Scan completed :\n", ifname);
      iw_init_event_stream(&stream, (char *) buffer, wrq.u.data.length);
      int count_i=0;
      do
	{
	  ret = iw_extract_event_stream(&stream, &iwe,
					range.we_version_compiled);
	  if(ret > 0)
	    count_i=print_scanning_token(&stream, &iwe, &state,
				 &range, has_range);



	}
      while(ret > 0);

      *ap_count=count_i;
      Printf("\n");
    }
  else
    Printf("%-8.16s  No scan results\n\n", ifname);

  free(buffer);
  return(0);
}

/************************老版本，未使用，注释掉*********************
//////////////////////////////////////////////设置wifi模块配置文件
void  set_wifi_config(int count,char * passwd)
{
	int fd;
	char buf[512];
	char ie[8];
	char ssid[32];

	ipc_net.wifi.current_ap = count;

	//记录下密码
	strcpy(ipc_net.wifi.wifiap[count].passwd, passwd);

	strcpy(ie, ipc_net.wifi.wifiap[count].iestat);
	strcpy(ssid, ipc_net.wifi.wifiap[count].name);
	memset(buf,0,512);
	fd=open("/etc/wpa_supplicant.conf",O_RDWR | O_CREAT | O_TRUNC,0777);
	if(fd < 0)
	{
		printf("open config err\n");
	}
	//wpa网络配置文件生成
	if(!(strcmp(ie,"wpa") && strcmp(ie,"wpa2")))
	{
		write(fd,"#This config created for wifi wpa/wpa2 link\n",strlen("#This config created for wifi wpa/wpa2 link\n"));
		strcpy(buf,"ctrl_interface_group=0\n");
		strcat(buf,"eapol_version=1\n");
		strcat(buf,"network={\nssid=\"");
		strcat(buf,ssid);
		strcat(buf,"\"\nkey_mgmt=WPA-PSK\nproto=WPA\npairwise=TKIP CCMP\npsk=\"");
		strcat(buf,passwd);
		strcat(buf,"\"\n}");
		write(fd,buf,strlen(buf));
		
	}
	//wep网络配置文件生成   未测试（没有找到可用的任何wep网络）
	else if(!strcmp(ie,"wep"))
	{
		write(fd,"#This config created for wifi wep link\n",strlen("#This config created for wifi wpa/wpa2 link\n"));
		strcpy(buf,"ctrl_interface_group=0\n");
		strcat(buf,"eapol_version=1\n");
		strcat(buf,"network={\nssid=\"");
		strcat(buf,ssid);
		strcat(buf,"\"\nkey_mgmt=NONE\nwep_key0=\"");
		strcat(buf,passwd);
		strcat(buf,"\"\n}");
		write(fd,buf,strlen(buf));
	}
	//明文网络配置文件生成
	else
	{
		write(fd,"#This config created for wifi free link\n",strlen("#This config created for wifi free link\n"));
		strcpy(buf,"ctrl_interface_group=0\n");
		strcat(buf,"network={\nssid=\"");
		strcat(buf,ssid);
		strcat(buf,"\"\nkey_mgmt=NONE\n}");
		write(fd,buf,strlen(buf));
	}
	
}
*************************************************************************/
//////////////////////////////////////////获取所有热点
void get_wifi_ap(int * ap, char* wifi)
{
	int i=1,ret;
	count=0;
	//memset(ipc_net.wifi.wifiap,0,sizeof(ipc_net.wifi.wifiap));
	printf("AP scanning with [%s]...\n", wifi);
	ret = print_scanning_info(&i, wifi, 0, 0);
	*ap=i;
	printf("%d AP searched\n", i);
}

int iwlist_get_ap_list(wifiap_t *aplist, char* wifi, int maxApCnt)
{
	int cnt = 0;

	get_wifi_ap(&cnt, wifi);
	cnt = cnt < maxApCnt ? cnt : maxApCnt;
	if (aplist == NULL)
		return 0;
	memset(aplist, 0, maxApCnt*sizeof(wifiap_t));
	memcpy(aplist, ipc_net.wifi.wifiap, cnt * sizeof(wifiap_t));
	return cnt;
}
/***************以前的接口，不再使用了，新接口在utl_ifconfig.c中****************
/////////////////////////////////////链接热点
void connect_ap(void)
{
	utl_system("wpa_supplicant -B -wlan0 -c /etc/wpa_supplicant.conf -Dwext");
	utl_system("udhcpc -i wlan0 &");
}


////////////////////////////////////断开链接，此处如果有 wpa_supplicant 运行，必须先结束这个进程，才能再次链接热点
void disconnect_ap(void)
{
	utl_system("killall wpa_supplicant");
	utl_system("killall udhcpc");
}
*/
