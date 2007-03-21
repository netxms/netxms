/* $Id: misc.cpp,v 1.1 2007-03-21 10:15:18 alk Exp $ */

#pragma warning(disable: 4786)

#include "nxhttpd.h"

using namespace std;

void Split(vector<string> &out, const string str, const string delim)
{
	string::size_type firstPos = 0;
	string::size_type secondPos = str.find_first_of(delim);

	out.push_back(str.substr(firstPos, secondPos));
	while (secondPos != string::npos)
	{
		firstPos = secondPos + 1;
		secondPos = str.find_first_of(delim, firstPos);
		out.push_back(str.substr(firstPos, secondPos - firstPos));
	}
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
