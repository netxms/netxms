/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxmarkdown.h
**
**/

#ifndef _nxmarkdown_h_
#define _nxmarkdown_h_

#include <nms_common.h>
#include <nms_util.h>

#ifndef LIBNXSRV_EXPORTABLE
#ifdef LIBNXSRV_EXPORTS
#define LIBNXSRV_EXPORTABLE __EXPORT
#else
#define LIBNXSRV_EXPORTABLE __IMPORT
#endif
#endif

/**
 * Target dialect for markdown to HTML conversion
 */
enum class MarkdownHTMLDialect
{
   GENERIC = 0,   // Full HTML with structural tags (h1-h6, p, ul/ol/li, blockquote, hr, table)
   TELEGRAM = 1   // Telegram bot API subset (b, i, s, code, pre, a, blockquote; headings, lists, and tables as preformatted text)
};

/**
 * Markdown conversion functions. Input and output are UTF-8. All functions accept a pragmatic
 * CommonMark subset (bold, italic, strikethrough, inline code, links, headings, lists with one
 * nesting level, fenced code blocks, blockquotes, horizontal rules) with GFM tables;
 * unrecognized syntax is passed through as literal text. Formats without native table support
 * (plain text, Telegram, Slack) get tables laid out as text with aligned columns. Returned
 * string is allocated with MemAlloc and should be freed by the caller.
 */
char LIBNXSRV_EXPORTABLE *MarkdownToPlainText(const char *markdown);
char LIBNXSRV_EXPORTABLE *MarkdownToHTML(const char *markdown, MarkdownHTMLDialect dialect);
char LIBNXSRV_EXPORTABLE *MarkdownToSlackText(const char *markdown);

#endif   /* _nxmarkdown_h_ */
