/* $Id: nxpie.h,v 1.2 2007-04-18 07:12:46 victor Exp $ */

#ifndef __NXPIE__H__
#define __NXPIE__H__

#pragma warning(disable: 4786)

#include <stdio.h>
#include <gd.h>
#include <gdfontt.h>
#include <stdlib.h>
#include <math.h>

#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#define snprintf _snprintf
#endif

using namespace std;

#define PI 3.141592653589793238462643

#define ROUND(x) (int)floor(x + 0.5)

enum
{
	MAX_PIE_ELEMENTS = 16,
	MAX_COLORS = 16,
};

enum
{	
	PIE_WIDTH = 400, // was: 320
	PIE_HEIGHT = 192,
};

class NxPie
{
public:
	NxPie();
	~NxPie();

	bool setValue(string label, double value);

	bool build(void);
	void free(void);
	void *getRawImage();
	int getRawImageSize();

private:
	vector<string> name;
	vector<double> value;
	void *rawData;
	int rawDataSize;

	int color[MAX_COLORS];
	int colorCount;
};

#endif // __NXPIE__H__

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.3  2006/03/03 22:29:17  victor
Windows portability fixes

Revision 1.2  2006/02/14 02:26:36  alk
implemented:

*) object list
*) last DCIs

Revision 1.1  2006/02/13 23:21:30  alk
*** empty log message ***

Revision 1.1  2006/02/13 23:15:12  alk
- login
- logout
- overview


*/
