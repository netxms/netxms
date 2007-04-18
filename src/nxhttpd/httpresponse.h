#ifndef __HTTP_RESPONSE__H__
#define __HTTP_RESPONSE__H__

#pragma warning(disable: 4786)

#include <map>
#include <string>

using namespace std;

/*
enum ResponseCode
{
	HTTP_UNKNOWN = 0,
	HTTP_OK = 200,
	HTTP_MOVEDPERMANENTLY = 301,
	HTTP_FOUND = 302,
	HTTP_UNAUTHORIZED = 401,
	HTTP_FORBIDDEN = 403,
	HTTP_NOTFOUND = 404,
	HTTP_INTERNALSERVERERROR = 500,
};
*/

enum ResponseCode
{
	HTTP_OK,
	HTTP_MOVEDPERMANENTLY,
	HTTP_FOUND,
	HTTP_BADREQUEST,
	HTTP_UNAUTHORIZED,
	HTTP_FORBIDDEN,
	HTTP_NOTFOUND,
	HTTP_INTERNALSERVERERROR,
};

class HttpResponse
{
public:
	HttpResponse();
	~HttpResponse();

	void SetHeader(const string, const string);
	void SetBody(std::string, bool = false);
	void SetBody(const char *, int, bool = false);
	void SetType(const string);
	void SetCode(int);

	string GetCodeString(void);
	
	char *BuildStream(int &);
	void CleanStream(char *);

private:
	int m_code;
	string m_codeString;
	map<string, string> m_headers;
	char *m_body;
	int m_bodyLen;
};

#endif // __HTTP_RESPONSE__H__
