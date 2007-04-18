#ifndef __HTTP_REQUEST__H__
#define __HTTP_REQUEST__H__

#pragma warning(disable: 4786)

#include <string>
#include <map>

enum
{
	METHOD_INVALID,
	METHOD_GET,
	METHOD_POST
};


class HttpRequest
{
public:
	HttpRequest();
	virtual ~HttpRequest();

	bool Read(SOCKET);

	bool GetQueryParam(const std::string, std::string &);
	void SetQueryParam(const std::string, std::string);
	std::string GetURI(void);

private:
	bool IsComplete(void);
	bool Parse(void);
	bool ParseQueryString(void);

	std::string m_raw;

	int method;
	std::string m_uri;
	std::string m_rawQuery;
	std::map<std::string, std::string> m_query;
};

#endif // __HTTP_REQUEST__H__
