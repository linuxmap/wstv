#include <string>

#ifndef _BASE64_H_
#define _BASE64_H_
void base64_encode_m(unsigned char const* bytes_to_encode, unsigned int in_len, char *dst); 
std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);

#endif


