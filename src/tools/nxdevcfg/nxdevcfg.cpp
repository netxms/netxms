/* 
** NetXMS - Network Management System
** Server startup module
** Copyright (C) 2003-2013 NetXMS Team
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxdevcfg.cpp
**
**/

#include <nms_util.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#define VALID_OPTIONS "bcCdilLPX"

/**
 * Externals
 */
extern const TCHAR *g_cFlags;
extern const TCHAR *g_cxxFlags;
extern const TCHAR *g_cppFlags;
extern const TCHAR *g_ldFlags;
extern const TCHAR *g_libs;

/**
 * Show help
 */
static void ShowHelp()
{
	printf("Available options:\n");
#if HAVE_DECL_GETOPT_LONG
	printf("   -b, --bindir    Binary directory\n"
	       "   -c, --cflags    C compiler flags\n"
	       "   -C, --cppflags  C/C++ compiler flags\n"
	       "   -X, --cxxflags  C++ compiler flags\n"
	       "   -d, --datadir   Data directory\n"
	       "   -l, --ldflags   Linker flags (all except -l)\n"
	       "   -L, --libdir    Library directory\n"
	       "   -i, --libs      Linker flags (only -l)\n"
	       "   -P, --prefix    Installation prefix\n"
	      );
#else
	printf("   -b  Binary directory\n"
	       "   -c  C compiler flags\n"
	       "   -C  C/C++ compiler flags\n"
	       "   -d  Data directory\n"
	       "   -i  Linker flags (only -l)\n"
	       "   -l  Linker flags (all except -l)\n"
	       "   -L  Library directory\n"
	       "   -P  Installation prefix\n"
	       "   -X  C++ compiler flags\n"
	      );
#endif
}

/**
 * Print "flags" string
 */
static void PrintFlags(const TCHAR *src)
{
	String s = src;
	s.translate(_T("${bindir}"), BINDIR);
	s.translate(_T("${libdir}"), LIBDIR);
	s.translate(_T("${pkgdatadir}"), DATADIR);
	s.translate(_T("${pkglibdir}"), PKGLIBDIR);
	s.translate(_T("${prefix}"), PREFIX);
	_tprintf(_T("%s\n"), (const TCHAR *)s);
}

/**
 * main
 */
int main(int argc, char *argv[])
{
#if HAVE_DECL_GETOPT_LONG
	static struct option longOptions[] = 
	{
		{ (char *)"bindir", 0, NULL, 'b' },
		{ (char *)"cflags", 0, NULL, 'c' },
		{ (char *)"cppflags", 0, NULL, 'C' },
		{ (char *)"cxxflags", 0, NULL, 'X' },
		{ (char *)"datadir", 0, NULL, 'd' },
		{ (char *)"ldflags", 0, NULL, 'l' },
		{ (char *)"libdir", 0, NULL, 'L' },
		{ (char *)"libs", 0, NULL, 'i' },
		{ (char *)"prefix", 0, NULL, 'P' },
		{ NULL, 0, 0, 0 }
	};
#endif
	int ch;

	if (argc < 2)
	{
		ShowHelp();
		return 1;
	}

#if HAVE_DECL_GETOPT_LONG
	while((ch = getopt_long(argc, argv, VALID_OPTIONS, longOptions, NULL)) != -1)
#else
	while((ch = getopt(argc, argv, VALID_OPTIONS)) != -1)
#endif
	{
		switch(ch)
		{
			case 'b':
				_tprintf(_T("%s\n"), BINDIR);
				return 0;
         case 'c':
            PrintFlags(g_cFlags);
            return 0;
         case 'C':
            PrintFlags(g_cppFlags);
            return 0;
         case 'X':
            PrintFlags(g_cxxFlags);
            return 0;
         case 'l':
            PrintFlags(g_ldFlags);
            return 0;
         case 'i':
            PrintFlags(g_libs);
            return 0;
			case 'd':
				_tprintf(_T("%s\n"), DATADIR);
				return 0;
			case 'L':
				_tprintf(_T("%s\n"), LIBDIR);
				return 0;
			case 'P':
				_tprintf(_T("%s\n"), PREFIX);
				return 0;
			case '?':
				ShowHelp();
				return 1;
			default:
				break;
		}
	}
	_tprintf(_T("\n"));
	return 0;
}
