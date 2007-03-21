/*
 * vars.h
 *
 * Some constant vars defined by default.
 *
 * libtpt 1.30
 *
 */

/*
 * Copyright (C) 2002-2004 Isaac W. Foraker (isaac@tazthecat.net)
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Author nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef include_libtpt_vars_h
#define include_libtpt_vars_h

// Ack!  A define!  Defines allow easy concatenation on strings
#define LIBTPT_VERSION	"1.30"

namespace TPT {

	const char* libtpt_version = LIBTPT_VERSION;
	const char* libtpt_library = "LibTPT";
	const char* libtpt_fullname =
		"LibTPT Template Processor Version " LIBTPT_VERSION;
	const char* libtpt_author = "Isaac W. Foraker";
	const char* libtpt_copyright =
		"Copyright (C) 2002-2004 Isaac W. Foraker, All Rights Reserved";
	const char* libtpt_license =
		"Copyright (C) 2002-2004 Isaac W. Foraker (isaac@tazthecat.net)\n"
		"All Rights Reserved\n"
		"\n"
		"Redistribution and use in source and binary forms, with or without\n"
		"modification, are permitted provided that the following conditions\n"
		"are met:\n"
		"\n"
		"1. Redistributions of source code must retain the above copyright\n"
		"   notice, this list of conditions and the following disclaimer.\n"
		"\n"
		"2. Redistributions in binary form must reproduce the above copyright\n"
		"   notice, this list of conditions and the following disclaimer in\n"
		"   the documentation and/or other materials provided with the\n"
		"   distribution.\n"
		"\n"
		"3. Neither the name of the Author nor the names of its contributors\n"
		"   may be used to endorse or promote products derived from this\n"
		"   software without specific prior written permission.\n"
		"\n"
		"THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''\n"
		"AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,\n"
		"THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A\n"
		"PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR\n"
		"CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n"
		"EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n"
		"PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n"
		"PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY\n"
		"OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n"
		"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
		"OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.";

	struct builtin_variables {
		const char* id;
		const char* value;
	} libtpt_builtins[] = {
		{ "template_version", libtpt_version },
		{ "template_library", libtpt_library },
		{ "template_fullname", libtpt_fullname },
		{ "template_author", libtpt_author },
		{ "template_copyright", libtpt_copyright },
		{ "template_license", libtpt_license },
		{ "VERSION", libtpt_version},
		{ "PML", libtpt_fullname }
	};

	const unsigned numbuiltins = (sizeof(libtpt_builtins)/sizeof(builtin_variables));
}

#endif // include_libtpt_vars_h
