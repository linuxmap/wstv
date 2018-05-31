
/*****************************************************************
 *
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 *
 ****************************************************************/


                                
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "pcs.h"

// Constants are the integer part of the sines of integers (in radians) * 2^32.
static const uint32_t k[64] = {
0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee ,
0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501 ,
0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be ,
0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821 ,
0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa ,
0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8 ,
0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed ,
0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a ,
0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c ,
0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70 ,
0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05 ,
0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665 ,
0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039 ,
0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1 ,
0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1 ,
0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };
 
// r specifies the per-round shift amounts
static const uint32_t r[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                      5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                      4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                      6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

static FILE *g_pcs_log;
static int g_log_level = 0; 					  
 
// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

/**
 * DEBUG macro, output the debug info to log file
 */
#define PCS_LOG_DEBUG(fmt, arg...) do { \
log_msg(16, "[DEBUG][%s:%d][FUNC:%s]" fmt,\
/*printf("[DEBUG]  [%s:%d][FUNC:%s]" fmt"\n",*/\
__FILE__, __LINE__, __FUNCTION__, ## arg); \
}while (0)

/**
 * WARNING macro, output the warning info to log file
 */
#define PCS_LOG_WARNING(fmt, arg...) do { \
log_msg(8, "[WARNING][%s:%d][FUNC:%s]" fmt,\
__FILE__,__LINE__,__FUNCTION__, ## arg); \
}while (0)

/**
 * ERROR macro, output the error info to log file
 */
#define PCS_LOG_ERROR(fmt, arg...) do { \
log_msg(4, "[ERROR]  [%s:%d][FUNC:%s]" fmt,\
/*printf("[ERROR]  [%s:%d][FUNC:%s]" fmt"\n",*/\
__FILE__, __LINE__, __FUNCTION__, ## arg); \
}while (0)

/** 
 * Normal URI prefix 
 */
static const char * PCS_URI_PREFIX = "https://pcs.baidu.com/rest/2.0/pcs/";

static const char * PCS_URI_SUFFIX_DEVICE = "device";

/**
 * PCS response  structure
 */
typedef struct _pcs_response_t
{
    unsigned long _http_status; /**< the http response code */
	unsigned long _header_size; /**< the http response header size */
	unsigned long _body_size;   /**< the http response body size */
    
	char _header[100*1024];     /**< the http response header */
	char _body[100*1024];   /**< the http response body */
} pcs_response_t;

static void to_bytes(uint32_t val, uint8_t *bytes)
{
    bytes[0] = (uint8_t) val;
    bytes[1] = (uint8_t) (val >> 8);
    bytes[2] = (uint8_t) (val >> 16);
    bytes[3] = (uint8_t) (val >> 24);
}
 
static uint32_t to_int32(const uint8_t *bytes)
{
    return (uint32_t) bytes[0]
        | ((uint32_t) bytes[1] << 8)
        | ((uint32_t) bytes[2] << 16)
        | ((uint32_t) bytes[3] << 24);
}
 
static void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest) 
{
 
    // These vars will contain the hash
    uint32_t h0, h1, h2, h3;
 
    // Message (to prepare)
    uint8_t *msg = NULL;
 
    size_t new_len, offset;
    uint32_t w[16];
    uint32_t a, b, c, d, i, f, g, temp;
 
    // Initialize variables - simple count in nibbles:
    h0 = 0x67452301;
    h1 = 0xefcdab89;
    h2 = 0x98badcfe;
    h3 = 0x10325476;
 
    //Pre-processing:
    //append "1" bit to message    
    //append "0" bits until message length in bits â‰¡ 448 (mod 512)
    //append length mod (2^64) to message
 
    for (new_len = initial_len + 1; new_len % (512/8) != 448/8; new_len++)
    {
    }

    msg = (uint8_t*)malloc(new_len + 8);
    memcpy(msg, initial_msg, initial_len);
    msg[initial_len] = 0x80; // append the "1" bit; most significant bit is "first"
    for (offset = initial_len + 1; offset < new_len; offset++)
    {
        msg[offset] = 0; // append "0" bits
    }
 
    // append the len in bits at the end of the buffer.
    to_bytes(initial_len*8, msg + new_len);
    // initial_len>>29 == initial_len*8>>32, but avoids overflow.
    to_bytes(initial_len>>29, msg + new_len + 4);
 
    // Process the message in successive 512-bit chunks:
    //for each 512-bit chunk of message:
    for(offset=0; offset<new_len; offset += (512/8)) 
    {
 
        // break chunk into sixteen 32-bit words w[j], 0 â‰¤ j â‰¤ 15
        for (i = 0; i < 16; i++)
        {
            w[i] = to_int32(msg + offset + i*4);
        }
 
        // Initialize hash value for this chunk:
        a = h0;
        b = h1;
        c = h2;
        d = h3;
 
        // Main loop:
        for(i = 0; i<64; i++) 
        {
 
            if (i < 16) 
            {
                f = (b & c) | ((~b) & d);
                g = i;
            } 
            else if (i < 32) 
            {
                f = (d & b) | ((~d) & c);
                g = (5*i + 1) % 16;
            } 
            else if (i < 48) 
            {
                f = b ^ c ^ d;
                g = (3*i + 5) % 16;          
            } 
            else 
            {
                f = c ^ (b | (~d));
                g = (7*i) % 16;
            }
 
            temp = d;
            d = c;
            c = b;
            b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
            a = temp;
 
        }
 
        // Add this chunk's hash to result so far:
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
 
    }
 
    // cleanup
    free(msg);
 
    //var char digest[16] := h0 append h1 append h2 append h3 //(Output is in little-endian)
    to_bytes(h0, digest);
    to_bytes(h1, digest + 4);
    to_bytes(h2, digest + 8);
    to_bytes(h3, digest + 12);
}

static int log_msg(int level, const char *format, ...) 
{
    if (level > g_log_level) 
    {
        return -1;
    }

    va_list ap;
    va_start(ap, format);

    char _time[100];
    time_t t = time(NULL);  
    strftime(_time, sizeof(_time), "%s, %d %b %Y %H:%M:%S GMT",gmtime(&t));
   
    char log_buf[2048];
    snprintf(log_buf, 2048, "[%s] %s\n", _time, format);

    vfprintf(g_pcs_log, log_buf, ap);
    va_end(ap); 
    return 0;
}

static void hex2string(unsigned char* src, char* dst, size_t n)
{
	unsigned char hb;
	unsigned char lb;
	size_t i = 0;
	
	for (;i < n;i++)
	{
		hb = (src[i] & 0xf0) >> 4;

		if (hb <= 9)
		{
			hb += 0x30;
		}
		else if (hb >= 10 && hb <= 15)
		{
			hb = hb -10 + 'a';
		}

		lb = src[i] & 0x0f;
		if (lb <= 9)
		{
			lb += 0x30;
		}
		else if ((int)lb >= 10 && (int)lb <= 15)
		{
			lb = lb - 10 + 'a';
		}

		dst[i*2]   = hb;
		dst[i*2+1] = lb;
	}
}

static void cal_sign(const char* deviceid, const char* access_token, uint8_t *digest, size_t digest_len)
{
    char ak[64];
    char appid[32];

    size_t ak_len = 0;
    size_t appid_len = 0;

    int ak_tag = 0;
    int app_tag = 0;

    int i = 0;

    size_t token_len = strlen(access_token);
    for (i = 0; i != token_len; ++i)
    {
        if ('.' == access_token[i])
        {
            if (0 == ak_tag)
            {
                ak_tag = 1;
            }
            else
            {
                ak_tag = 2;
                ak[ak_len] = 0;
            }
        }   
        else 
        {
            if (1 == ak_tag)
            {
                ak[ak_len++] = access_token[i];             
            }
        }

        if ('-' == access_token[i])
        {
            if (0 == app_tag)
            {
                app_tag = 1;
            }
        }
        else 
        {
            if (1 == app_tag)
            {
                appid[appid_len++] = access_token[i];
            }
        }
    }

    appid[appid_len] = 0;

    char origin_msg[256];
    strncpy(origin_msg, deviceid, strlen(deviceid));
    strncat(origin_msg, appid, appid_len);
    strncat(origin_msg, ak, ak_len);
    char* sk = "sf0RTtUgxAhNDG6iDFnfhmqa9w60kH1B";
    strncat(origin_msg, sk, strlen(sk));

	uint8_t md5_var[17];
    md5((const uint8_t* )origin_msg, strlen(origin_msg), md5_var);

	hex2string(md5_var, (char*)digest, 16);
	
    digest[digest_len - 1] = 0;

    PCS_LOG_DEBUG("ak[%s] appid[%s] digest[%s]", ak, appid, digest);
}

/**
 * The callback function, to write the HTTP response body to pcs response body
 */
static size_t write_body(void *ptr, size_t size, size_t nmemb, pcs_response_t *s)
{
	size_t len = size*nmemb;
	memcpy(s->_body, ptr, len);
	s->_body[len] = '\0';
	s->_body_size = len;

	return size*nmemb;
}

/**                                                                                                                                                                                                            
 * The Callback function, to write the HTTP response header to pcs response header
 */
static size_t write_header(void *ptr, size_t size, size_t nmemb, pcs_response_t *s)
{
	size_t len = size*nmemb;
	memcpy(s->_header+s->_header_size, ptr, len);
	s->_header_size += len;
	s->_header[s->_header_size] = '\0';

	return size*nmemb;
}


/**
 * check the HTTP status code
 */
static int is_ok(long res_code)
{          
	//ÔÝÊ±ÏÈ²»ÓÃ£¬code 401Ò²¿ÉÒÔ
	return 0;
    if (res_code == 200 || res_code == 201 || res_code ==204 || res_code == 206)
    {
    PCS_LOG_DEBUG("HTTP succeed, HTTP status code is [%ld]\n", res_code);
    return 0;
    }
    else
    {
        PCS_LOG_ERROR("HTTP Failed, HTTP  status code is [%ld]\n", res_code);
    return 1;
    }
}

static int curl_request(const char* request_url, pcs_response_t* pcs_res)     
{                                                                                                      
    CURLcode res;                                                                                      
    CURL* curl;                                                                                        
	
    curl_global_init(CURL_GLOBAL_DEFAULT);                                                             
    curl = curl_easy_init();                                                                           
    if (NULL == curl)                                                                                  
    {                                                                                                  
        PCS_LOG_ERROR("Curl_easy_init error\n");                                                       
        return 1;                                                                                      
    }                
	printf("url: %s\n", request_url);
	
    curl_easy_setopt(curl, CURLOPT_URL, request_url);                               
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);                                                       
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_body);                                         
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, pcs_res);                                       
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header);                                      
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, pcs_res);                                     
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10000L);                                                   
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 60L);  
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);	
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
	
    PCS_LOG_DEBUG("Exec http request, request url[%s]", request_url);   
    res = curl_easy_perform(curl);                                                                     
    if (res != CURLE_OK)                                                                               
    {                                                                                                  
        PCS_LOG_ERROR("Curl easy perform error, info:[%s]\n", curl_easy_strerror(res));                
        goto error_process;                                                                            
    }                                                                                                  
	
    res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(pcs_res->_http_status));          
    if (res != CURLE_OK)                                                                               
    {                                                                                                  
        PCS_LOG_ERROR("Curl easy getinfo error, info:[%s]", curl_easy_strerror(res));                  
        goto error_process;                                                                            
    }                                                                                                  
	
    res = is_ok(pcs_res->_http_status);                                                        
	
    curl_easy_cleanup(curl);                                                                           
    curl = NULL;                                                                                       
    curl_global_cleanup();
	
    return res;                                                                                        
	
error_process:                                                                                         
    curl_easy_cleanup(curl);                                                                           
    curl = NULL;                                                                                       
    curl_global_cleanup();                                                                             
    return 1;                                                                                          
}  

/**     
 * Init a pcs response structure
 */ 
static int pcs_res_init(pcs_response_t *pcs_res)
{
    if (NULL == pcs_res)
    {
        PCS_LOG_ERROR("Init the pcs_response_t structure error, pointer is NULL!\n");
        return 1;
    }
    PCS_LOG_DEBUG("Init pcs response structure\n");
    memset(pcs_res, 0, sizeof(pcs_response_t));
	
    return 0;
}

/**
 * Destroy a pcs response structure
 */
static int pcs_res_destroy(pcs_response_t *pcs_res)
{
    if (NULL != pcs_res)
    {
    PCS_LOG_DEBUG("Destroy a pcs response structure\n");
    memset(pcs_res, 0, sizeof(pcs_response_t));
        return 0;
    }
    PCS_LOG_ERROR("Destory the pcs_response_t structure error, pointer is NULL!\n");
    return 1;
}

int open_log(const char* file_path, int log_level)
{
	if (log_level > LOG_DEBUG || log_level < LOG_ERROR)
	{
		fprintf(stderr, "open log failed: param error\n");
		return 1;
	}
	
    if (NULL == file_path)
    {
		g_pcs_log = stderr;
    }
    else
    {
		g_pcs_log = fopen(file_path, "a+");
		if (NULL == g_pcs_log)
		{
			fprintf(stderr, "Log init error, can't open file [%s]! Set the log to stdout\n", file_path);
			g_pcs_log = stderr;
			return 1;
		}
    }
	
	g_log_level = log_level;
	
    return 0;	   
}

int close_log()
{
    if (NULL != g_pcs_log)
    {
		PCS_LOG_DEBUG("close log handle\n");
		int res = fclose(g_pcs_log);
        if (res)
        {
            PCS_LOG_ERROR("close log handle error, info:[%s]\n", strerror(errno));
            return 1;
        }
		
        g_pcs_log = NULL;
    }
   
	return 0;
}

int locate_upload(const char* deviceid, const char* streamid, char* result, unsigned long* ulen)
{
	if (NULL == deviceid || NULL == streamid || NULL == result || 0 == *ulen)
	{
		PCS_LOG_ERROR("locateupload param error");
		return 1;
	}
	
    pcs_response_t locateupload_res;
	
	char request_url[1024];
	memset(request_url, 0, sizeof(request_url));
	
	snprintf(request_url, sizeof(request_url), "%s%s%s%s%s%s", PCS_URI_PREFIX, PCS_URI_SUFFIX_DEVICE, "?method=locateuploadex&deviceid=",
		deviceid, "&streamid=", streamid);
	
    pcs_res_init(&locateupload_res);
	
	int res = 0;
    res = curl_request(request_url, &locateupload_res);
    if (res)
    {
        char log[1024];
        strncpy(log, locateupload_res._header, 1024);
        
		PCS_LOG_ERROR("locateupload perform error:%s", log);
    }
    else
    {
        PCS_LOG_DEBUG("locateupload perform success");
    }

    if (*ulen <= locateupload_res._body_size) 
    {
        PCS_LOG_ERROR("ulen[%ul] is less than body size[%ul]", (unsigned int)*ulen, (unsigned int)locateupload_res._body_size);

        res = 1;
    }
    else 
    {
        *ulen = locateupload_res._body_size;
        strncpy(result, locateupload_res._body, *ulen);
		result[*ulen] = 0;
    }

    pcs_res_destroy(&locateupload_res);

    return res;
}

int get_user_token(const char* deviceid, const char* access_token, char* result, unsigned long* ulen)
{
	if (NULL == deviceid || NULL == access_token || NULL == result || 0 == *ulen)
	{
		PCS_LOG_ERROR("get_user_token param error");
		return 1;		
	}

	uint8_t digest[33];
	cal_sign(deviceid, access_token, digest, 33);
	
    pcs_response_t getusertoken_res ;
	pcs_res_init(&getusertoken_res);
    
	char request_url[1024];
	memset(request_url, 0, sizeof(request_url));
	/*
    strncpy(request_url, PCS_URI_PREFIX, strlen(PCS_URI_PREFIX));
    strncat(request_url, PCS_URI_SUFFIX_DEVICE, strlen(PCS_URI_SUFFIX_DEVICE));

    strncat(request_url, "?method=getusertoken&deviceid=", sizeof("?method=getusertoken&deviceid="));
    strncat(request_url, deviceid, strlen(deviceid));
 
    strncat(request_url, "&access_token=", strlen("&access_token="));
    strncat(request_url, access_token, strlen(access_token));

    strncat(request_url, "&sign=", strlen("&sign="));
    strncat(request_url, digest, strlen(digest));	
    */
	snprintf(request_url, sizeof(request_url), "%s%s%s%s%s%s%s%s", PCS_URI_PREFIX, PCS_URI_SUFFIX_DEVICE, "?method=getusertoken&deviceid=",
		deviceid, "&access_token=", access_token, "&sign=", digest);
	
	int res = 0;
	
    res = curl_request(request_url, &getusertoken_res);
    if (res)
    {
		PCS_LOG_ERROR("get user token perform error");
    }
    else 
    {
        PCS_LOG_ERROR("get user token success");
    }
   
    if (*ulen <= getusertoken_res._body_size) 
    {
        PCS_LOG_ERROR("ulen[%ul] is less than body size[%ul]", (unsigned int)*ulen, (unsigned int)getusertoken_res._body_size);

        res = 1;
    }
    else 
    {
        *ulen = getusertoken_res._body_size;
        strncpy(result, getusertoken_res._body, *ulen);
		result[*ulen] = 0;
    }

    pcs_res_destroy(&getusertoken_res);
  
    return res; 
}

int get_conn_token(const char* deviceid, const char* access_token, char* result, unsigned long* ulen)
{
	if (NULL == deviceid || NULL == access_token || NULL == result || 0 == *ulen)
	{
		PCS_LOG_ERROR("get_conn_token param error");
		return 1;		
	}
	
    pcs_response_t getconntoken_res ;
	pcs_res_init(&getconntoken_res);
    
	char request_url[1024];
	memset(request_url, 0, sizeof(request_url));
	
	snprintf(request_url, sizeof(request_url), "%s%s%s%s%s%s", PCS_URI_PREFIX, PCS_URI_SUFFIX_DEVICE, "?method=getconntoken&deviceid=",
		deviceid, "&access_token=", access_token);
	
	int res = 0;
	
    res = curl_request(request_url, &getconntoken_res);
    if (res)
    {
		PCS_LOG_ERROR("get conn token perform error");
    }
    else 
    {
        PCS_LOG_ERROR("get conn token success");
    }
   
    if (*ulen <= getconntoken_res._body_size) 
    {
        PCS_LOG_ERROR("ulen[%ul] is less than body size[%ul]", (unsigned int)*ulen, (unsigned int)getconntoken_res._body_size);

        res = 1;
    }
    else 
    {
        *ulen = getconntoken_res._body_size;
        strncpy(result, getconntoken_res._body, *ulen);
		result[*ulen] = 0;
    }

    pcs_res_destroy(&getconntoken_res);
  
    return res; 
}
