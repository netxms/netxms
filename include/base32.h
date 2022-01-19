/* base32.h -- Encode binary data using printable characters.
   Copyright (C) 2004-2006, 2009-2022 Free Software Foundation, Inc.
   Adapted from Simon Josefsson's base64 code by Gijs van Tulder.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#ifndef BASE32_H
#define BASE32_H

/* This uses that the expression (n+(k-1))/k means the smallest
   integer >= n/k, i.e., the ceiling of n/k.  */
#define BASE32_LENGTH(inlen) ((((inlen) + 4) / 5) * 8)

bool LIBNETXMS_EXPORTABLE isbase32(char ch);

void LIBNETXMS_EXPORTABLE base32_encode(const char *in, size_t inlen, char *out, size_t outlen);

size_t LIBNETXMS_EXPORTABLE base32_encode_alloc(const char *in, size_t inlen, char **out);

bool LIBNETXMS_EXPORTABLE base32_decode(const char *in, size_t inlen, char *out, size_t *outlen);

bool LIBNETXMS_EXPORTABLE base32_decode_alloc(const char *in, size_t inlen, char **out, size_t *outlen);

#endif /* BASE32_H */
