/*
 * EncodeConvertor.h
 *
 *  Created on: 21 апреля 2017.
 *      Author: Denis Kashitsin
 */

#ifndef SRC_APPL_ENCODECONVERTOR_H_
#define SRC_APPL_ENCODECONVERTOR_H_

#include <iconv.h>
#include <errno.h>
#include <err.h>
#include <string>

using namespace std;

class EncodeConvertor {
private:
	string convert(iconv_t cd, const string &val, int *err);
	string conv(const char to[], const char from[], const string &val, int *err);

public:
	EncodeConvertor();
	virtual ~EncodeConvertor();

	string UTF8toCP866(const string &val, int *err);
	string CP866toUTF8(const string &val, int *err);
};

#endif /* SRC_APPL_ENCODECONVERTOR_H_ */
