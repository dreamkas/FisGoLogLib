/*
 * EncodeConvertor.h
 */

#include "EncodeConvertor.h"
#include <string.h>
//#include "log.h"

//#include "convert.h"
#include "errno.h"

EncodeConvertor::EncodeConvertor() {
	// TODO Auto-generated constructor stub
}

EncodeConvertor::~EncodeConvertor() {
	// TODO Auto-generated destructor stub
}


//-----------------------------------------------


string charToString(const char *source)
{
	int size = strlen(source);
	string str(source, size);
	return str;
}

void stringToChar( string source, char *dest, uint32_t max_size)
{
	string strDest;
	uint32_t max = source.length();
	if ((max) > max_size)
	{
		max = max_size;
	}
	memcpy( dest, source.c_str(), max);
}

//Метод удаления русских кавычек из UTF8
string replaceSymbols(const string val)
{
	string sResult;
	char tmpStrC[1024];
	stringToChar(val, tmpStrC, val.length());

	for(unsigned int i = 1; i < val.length(); i++)
	{
		if((tmpStrC[i] == 171) || (tmpStrC[i] == 187))
		{
			if(tmpStrC[i-1] == 194)
			{
				tmpStrC[i-1] = '\"';
				memcpy(&tmpStrC[i], &tmpStrC[i+1],strlen(tmpStrC)-i);
			}
		}
	}
	sResult = charToString(tmpStrC);
	return sResult;
}

//-----------------------------------------------
string EncodeConvertor::convert(iconv_t cd, const string &val, int* err)
{
	if(cd == (iconv_t)-1) return "";
	const size_t INBUFSIZE  = val.length() + 1;
	const size_t OUTBUFSIZE = 16384;
	size_t x = INBUFSIZE;

	char *in_buf = new char[INBUFSIZE];
	strncpy(in_buf, val.c_str(), val.length());
	in_buf[val.length()] = 0;
	const char * tbf = in_buf;
	string res;
	while(x)
	{
		size_t con;
		char out_buf[OUTBUFSIZE + 1];
		char *pout_buf = out_buf;
		memset(pout_buf,0,OUTBUFSIZE + 1);

		size_t y = OUTBUFSIZE;
		con = iconv(cd, (char**)&tbf, &x, &pout_buf, &y);
		res += out_buf;
		if(con == (size_t)-1)
        {
			*err = 1;
        	res.clear();
            res = "";
            x = 0;
        }
	}
	delete [] in_buf;
	return res;
}

string EncodeConvertor::conv(const char to[], const char from[], const string &val, int* err)
{
	*err = 0;
	iconv_t cd = iconv_open(to, from);
	std::string r = convert(cd, val, err);
	if(*err != 0)
	{
		printf("LOG CONVERTER(to UTF8):Converter Error with %s", val.c_str());
		*err = 0;
	    string tmpStr;
	    tmpStr = replaceSymbols(val);
		r = convert(cd, tmpStr, err);
	}
	iconv_close(cd);
	return r;
}

string EncodeConvertor::UTF8toCP866(const string &val, int* err)
{
	if(*err == 0)
	{
	    if(val.length() > 0) return conv( "CP866//IGNORE", "UTF-8", val, err);
	}
    return "";
}

string EncodeConvertor::CP866toUTF8(const string &val, int* err)
{
	if(*err == 0)
	{
	    if(val.length() > 0) return conv("UTF-8", "CP866", val, err);
	}
	return "";
}
