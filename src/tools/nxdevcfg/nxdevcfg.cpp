/* 
** NetXMS - Network Management System
** Development configuration helper
** Copyright (C) 2003-2025 Raden Solutions
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
#include <netxms_getopt.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxdevcfg)

#define VALID_OPTIONS "bcCdDeEijlLoOpPStTuUX"

/**
 * Externals
 */
extern const TCHAR *g_cFlags;
extern const TCHAR *g_cxxFlags;
extern const TCHAR *g_cppFlags;
extern const TCHAR *g_ldFlags;
extern const TCHAR *g_execLdFlags;
extern const TCHAR *g_libs;
extern const TCHAR *g_execLibs;
extern const TCHAR *g_cc;
extern const TCHAR *g_cxx;
extern const TCHAR *g_ld;
extern const TCHAR *g_perl;
extern const TCHAR *g_serverLibs;
extern const TCHAR *g_tuxedoCppFlags;
extern const TCHAR *g_tuxedoLdFlags;
extern const TCHAR *g_tuxedoLibs;

/**
 * Show help
 */
static void ShowHelp()
{
	printf("Available options:\n");
#if HAVE_DECL_GETOPT_LONG
	printf("   -b, --bindir          Binary directory\n"
	       "   -o, --cc              C compiler\n"
	       "   -c, --cflags          C compiler flags\n"
	       "   -C, --cppflags        C/C++ compiler flags\n"
	       "   -u, --curl-libs       Linker flags for using cURL\n"
	       "   -O, --cxx             C++ compiler\n"
	       "   -X, --cxxflags        C++ compiler flags\n"
	       "   -d, --datadir         Data directory\n"
	       "   -e, --exec-ldflags    Linker flags (all except -l) for executables\n"
	       "   -E, --exec-libs       Linker flags (only -l) for executables\n"
	       "   -j, --ijansson-libs   Linker flags for using libjansson\n"
	       "   -D, --ld              Linker\n"
	       "   -l, --ldflags         Linker flags (all except -l)\n"
	       "   -L, --libdir          Library directory\n"
	       "   -i, --libs            Linker flags (only -l)\n"
	       "   -p, --perl            Perl interpreter\n"
	       "   -P, --prefix          Installation prefix\n"
	       "   -S, --server-libs     Linker flags for server binaries (only -l)\n"
          "   -t, --tuxedo-cppflags Tuxedo related compiler flags\n"
          "   -T, --tuxedo-ldflags  Tuxedo related linker flags (all except -l)\n"
          "   -U, --tuxedo-libs     Tuxedo related linker flags (only -l)\n"
	      );
#else
	printf("   -b  Binary directory\n"
	       "   -c  C compiler flags\n"
	       "   -C  C/C++ compiler flags\n"
	       "   -d  Data directory\n"
	       "   -D  Linker\n"
	       "   -e  Linker flags (all except -l) for executables\n"
	       "   -E  Linker flags (only -l) for executables\n"
	       "   -i  Linker flags (only -l)\n"
	       "   -j  Linker flags for using libjansson\n"
	       "   -l  Linker flags (all except -l)\n"
	       "   -L  Library directory\n"
	       "   -o  C compiler\n"
	       "   -O  C++ compiler\n"
	       "   -p  Perl interpreter\n"
	       "   -P  Installation prefix\n"
	       "   -S  Linker flags for server binaries (only -l)\n"
          "   -t  Tuxedo related compiler flags\n"
          "   -T  Tuxedo related linker flags (all except -l)\n"
	       "   -u  Linker flags for using cURL\n"
          "   -U  Tuxedo related linker flags (only -l)\n"
	       "   -X  C++ compiler flags\n"
	      );
#endif
}

/**
 * Print "flags" string
 */
static void PrintFlags(const TCHAR *src)
{
	StringBuffer s = src;
	s.replace(_T("${bindir}"), BINDIR);
	s.replace(_T("${libdir}"), LIBDIR);
	s.replace(_T("${localstatedir}"), STATEDIR);
	s.replace(_T("${pkgdatadir}"), DATADIR);
	s.replace(_T("${pkglibdir}"), PKGLIBDIR);
	s.replace(_T("${prefix}"), PREFIX);
	s.escapeCharacter(_T('"'), _T('\\'));
	_tprintf(_T("%s\n"), s.cstr());
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
		{ (char *)"cc", 0, NULL, 'o' },
		{ (char *)"cflags", 0, NULL, 'c' },
		{ (char *)"cppflags", 0, NULL, 'C' },
		{ (char *)"curl-libs", 0, NULL, 'u' },
		{ (char *)"cxx", 0, NULL, 'O' },
		{ (char *)"cxxflags", 0, NULL, 'X' },
		{ (char *)"datadir", 0, NULL, 'd' },
		{ (char *)"exec-ldflags", 0, NULL, 'e' },
		{ (char *)"exec-libs", 0, NULL, 'E' },
		{ (char *)"jansson-libs", 0, NULL, 'j' },
		{ (char *)"ld", 0, NULL, 'D' },
		{ (char *)"ldflags", 0, NULL, 'l' },
		{ (char *)"libdir", 0, NULL, 'L' },
		{ (char *)"libs", 0, NULL, 'i' },
		{ (char *)"perl", 0, NULL, 'p' },
		{ (char *)"prefix", 0, NULL, 'P' },
		{ (char *)"server-libs", 0, NULL, 'S' },
		{ (char *)"tuxedo-cppflags", 0, NULL, 't' },
		{ (char *)"tuxedo-ldflags", 0, NULL, 'T' },
		{ (char *)"tuxedo-libs", 0, NULL, 'U' },
		{ NULL, 0, 0, 0 }
	};
#endif
	int ch;

   InitNetXMSProcess(true);

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
			case 'd':
				_tprintf(_T("%s\n"), DATADIR);
				return 0;
			case 'D':
				_tprintf(_T("%s\n"), g_ld);
				return 0;
			case 'e':
				_tprintf(_T("%s\n"), g_execLdFlags);
				return 0;
			case 'E':
				_tprintf(_T("%s\n"), g_execLibs);
				return 0;
         case 'i':
            PrintFlags(g_libs);
            return 0;
         case 'j':
#if BUNDLED_LIBJANSSON
				_tprintf(_T("-lnxjansson\n"));
#else
				_tprintf(_T("-ljansson\n"));
#endif
            return 0;
         case 'l':
            PrintFlags(g_ldFlags);
            return 0;
			case 'L':
				_tprintf(_T("%s\n"), LIBDIR);
				return 0;
			case 'o':
				_tprintf(_T("%s\n"), g_cc);
				return 0;
			case 'O':
				_tprintf(_T("%s\n"), g_cxx);
				return 0;
			case 'p':
				_tprintf(_T("%s\n"), g_perl);
				return 0;
			case 'P':
				_tprintf(_T("%s\n"), PREFIX);
				return 0;
			case 'S':
				_tprintf(_T("%s\n"), g_serverLibs);
				return 0;
			case 't':
				_tprintf(_T("%s\n"), g_tuxedoCppFlags);
				return 0;
			case 'T':
				_tprintf(_T("%s\n"), g_tuxedoLdFlags);
				return 0;
         case 'u':
				_tprintf(_T("-lcurl\n"));
            return 0;
			case 'U':
				_tprintf(_T("%s\n"), g_tuxedoLibs);
				return 0;
         case 'X':
            PrintFlags(g_cxxFlags);
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
