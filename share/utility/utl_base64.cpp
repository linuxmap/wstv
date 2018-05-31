#include <string>

#include "base64.h"
#include "utl_base64.h"

void utl_base64_encode_m(unsigned char const* bytes_to_encode, unsigned int in_len, char *dst)
{
	base64_encode_m(bytes_to_encode, in_len, dst);
}

char const* utl_base64_encode(unsigned char const* bytes_to_encode, unsigned int len)
{
	static std::string encode_str = base64_encode(bytes_to_encode, len);

	return encode_str.c_str();
}

char const* utl_base64_decode(char const* s)
{
	std::string src_str = s;
	static std::string decoded_str = base64_decode(s);

	return decoded_str.c_str();
}

