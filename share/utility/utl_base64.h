#ifndef _UTL_BASE64_H_
#define _UTL_BASE64_H_


#ifdef __cplusplus
extern "C" {
#endif

void utl_base64_encode_m(unsigned char const* bytes_to_encode, unsigned int in_len, char *dst); 

// char const* utl_base64_encode(unsigned char const* bytes_to_encode, unsigned int len);

// 该函数不可重入
char const* utl_base64_decode(char const* s);

#ifdef __cplusplus
}
#endif

#endif
