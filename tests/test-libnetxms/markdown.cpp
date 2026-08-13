/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: markdown.cpp
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <testtools.h>
#include <nxmarkdown.h>

/**
 * Helpers - run conversion and compare result
 */
static void CheckPlain(const char *input, const char *expected)
{
   char *r = MarkdownToPlainText(input);
   AssertEquals(r, expected);
   MemFree(r);
}

static void CheckTelegram(const char *input, const char *expected)
{
   char *r = MarkdownToHTML(input, MarkdownHTMLDialect::TELEGRAM);
   AssertEquals(r, expected);
   MemFree(r);
}

static void CheckGeneric(const char *input, const char *expected)
{
   char *r = MarkdownToHTML(input, MarkdownHTMLDialect::GENERIC);
   AssertEquals(r, expected);
   MemFree(r);
}

static void CheckSlack(const char *input, const char *expected)
{
   char *r = MarkdownToSlackText(input);
   AssertEquals(r, expected);
   MemFree(r);
}

static void CheckTerminal(const char *input, const char *expected)
{
   char *r = MarkdownToTerminal(input);
   AssertEquals(r, expected);
   MemFree(r);
}

/**
 * Test markdown conversion functions
 */
void TestMarkdown()
{
   StartTest(_T("Markdown - inline emphasis"));
   CheckPlain("Hello **world**!", "Hello world!");
   CheckTelegram("Hello **world**!", "Hello <b>world</b>!");
   CheckGeneric("Hello **world**!", "<p>Hello <b>world</b>!</p>");
   CheckSlack("Hello **world**!", "Hello *world*!");
   CheckTelegram("*emph* and _also_", "<i>emph</i> and <i>also</i>");
   CheckSlack("*emph* and _also_", "_emph_ and _also_");
   CheckPlain("*emph* and _also_", "emph and also");
   CheckTelegram("~~gone~~", "<s>gone</s>");
   CheckSlack("~~gone~~", "~gone~");
   CheckPlain("~~gone~~", "gone");
   CheckTelegram("***both***", "<b><i>both</i></b>");
   CheckTelegram("**bold with *nested* inside**", "<b>bold with <i>nested</i> inside</b>");
   EndTest();

   StartTest(_T("Markdown - code spans"));
   CheckTelegram("run `nxdbmgr check` now", "run <code>nxdbmgr check</code> now");
   CheckSlack("run `nxdbmgr check` now", "run `nxdbmgr check` now");
   CheckPlain("run `nxdbmgr check` now", "run nxdbmgr check now");
   CheckTelegram("escape `a < b` inside", "escape <code>a &lt; b</code> inside");
   CheckTelegram("`123456`", "<code>123456</code>");   // 2FA code (issue #2478) - single backtick must not start fenced block
   CheckPlain("`123456`", "123456");
   EndTest();

   StartTest(_T("Markdown - links"));
   CheckPlain("See [docs](https://netxms.org) now", "See docs (https://netxms.org) now");
   CheckTelegram("See [docs](https://netxms.org) now", "See <a href=\"https://netxms.org\">docs</a> now");
   CheckGeneric("See [docs](https://netxms.org) now", "<p>See <a href=\"https://netxms.org\">docs</a> now</p>");
   CheckSlack("See [docs](https://netxms.org) now", "See <https://netxms.org|docs> now");
   CheckPlain("[https://netxms.org](https://netxms.org)", "https://netxms.org");
   EndTest();

   StartTest(_T("Markdown - autolinks"));
   CheckPlain("See <https://netxms.org> now", "See https://netxms.org now");
   CheckGeneric("See <https://netxms.org> now", "<p>See <a href=\"https://netxms.org\">https://netxms.org</a> now</p>");
   CheckTelegram("See <https://netxms.org> now", "See <a href=\"https://netxms.org\">https://netxms.org</a> now");
   CheckSlack("See <https://netxms.org> now", "See <https://netxms.org|https://netxms.org> now");
   CheckPlain("Mail <admin@example.com> now", "Mail admin@example.com now");
   CheckGeneric("Mail <admin@example.com> now", "<p>Mail <a href=\"mailto:admin@example.com\">admin@example.com</a> now</p>");
   // Angle brackets that are not autolinks stay literal text
   CheckGeneric("Tag <b>bold</b>, <notaurl>, <a b>, <x@y>, <>",
         "<p>Tag &lt;b&gt;bold&lt;/b&gt;, &lt;notaurl&gt;, &lt;a b&gt;, &lt;x@y&gt;, &lt;&gt;</p>");
   CheckTelegram("Unclosed <https://x and text", "Unclosed &lt;https://x and text");
   EndTest();

   StartTest(_T("Markdown - escaping"));
   CheckTelegram("5 < 6 & 7 > 2", "5 &lt; 6 &amp; 7 &gt; 2");
   CheckGeneric("5 < 6 & 7 > 2", "<p>5 &lt; 6 &amp; 7 &gt; 2</p>");
   CheckSlack("5 < 6 & 7 > 2", "5 &lt; 6 &amp; 7 &gt; 2");
   CheckPlain("5 < 6 & 7 > 2", "5 < 6 & 7 > 2");
   EndTest();

   StartTest(_T("Markdown - literal pass-through"));
   CheckTelegram("value of var_name_one is 5", "value of var_name_one is 5");
   CheckPlain("2 * 3 * 4 = 24", "2 * 3 * 4 = 24");
   CheckTelegram("a ** b", "a ** b");
   CheckTelegram("**unclosed bold", "**unclosed bold");
   CheckTelegram("\\*not emphasis\\*", "*not emphasis*");
   CheckPlain("CPU > 90% (rising)", "CPU > 90% (rising)");
   EndTest();

   StartTest(_T("Markdown - headings"));
   CheckPlain("# Title\n\nBody text", "Title\n\nBody text");
   CheckTelegram("# Title\n\nBody text", "<b>Title</b>\n\nBody text");
   CheckSlack("# Title\n\nBody text", "*Title*\n\nBody text");
   CheckGeneric("## Sub ##\n\nBody", "<h2>Sub</h2>\n<p>Body</p>");
   EndTest();

   StartTest(_T("Markdown - lists"));
   CheckPlain("Items:\n- one\n- two\n  - nested\n- three",
         "Items:\n\n- one\n- two\n   - nested\n- three");
   CheckTelegram("- one\n- two",
         "\xE2\x80\xA2 one\n\xE2\x80\xA2 two");
   CheckPlain("1. first\n2. second", "1. first\n2. second");
   CheckGeneric("- one\n- two", "<ul>\n<li>one</li>\n<li>two</li>\n</ul>");
   CheckGeneric("1. first\n2. second", "<ol>\n<li>first</li>\n<li>second</li>\n</ol>");
   // Ordered list not starting from 1
   CheckPlain("5. five\n6. six", "5. five\n6. six");
   CheckGeneric("5. five\n6. six", "<ol start=\"5\">\n<li>five</li>\n<li>six</li>\n</ol>");
   // Wrapped list items - continuation lines belong to preceding item
   CheckPlain("- item one\n  continued here\n- next item", "- item one\n  continued here\n- next item");
   CheckTelegram("- item one\n  continued here\n- next item",
         "\xE2\x80\xA2 item one\n  continued here\n\xE2\x80\xA2 next item");
   CheckGeneric("- item one\n  continued here\n- next item",
         "<ul>\n<li>item one<br/>\ncontinued here</li>\n<li>next item</li>\n</ul>");
   CheckPlain("1. first line\n   wrapped part\n2. second", "1. first line\n   wrapped part\n2. second");
   CheckPlain("- one\n- two\n  - nested\n    nested wrap\n- three",
         "- one\n- two\n   - nested\n     nested wrap\n- three");
   CheckGeneric("- item\n  continued\n\nParagraph", "<ul>\n<li>item<br/>\ncontinued</li>\n</ul>\n<p>Paragraph</p>");
   EndTest();

   StartTest(_T("Markdown - code blocks"));
   CheckPlain("```json\n{ \"a\": 1 }\n```", "{ \"a\": 1 }");
   CheckTelegram("```json\n{ \"a\": 1 }\n```", "<pre><code class=\"language-json\">{ \"a\": 1 }</code></pre>");
   CheckTelegram("```\nplain <code>\n```", "<pre>plain &lt;code&gt;</pre>");
   CheckSlack("```\nline1\nline2\n```", "```\nline1\nline2\n```");
   CheckGeneric("```json\n{ \"a\": 1 }\n```", "<pre><code class=\"language-json\">{ \"a\": 1 }\n</code></pre>");
   CheckTelegram("```\n**not bold**\n```", "<pre>**not bold**</pre>");
   CheckPlain("```\ndangling", "dangling");
   EndTest();

   StartTest(_T("Markdown - blockquotes"));
   CheckTelegram("> quoted line\n> second", "<blockquote>quoted line\nsecond</blockquote>");
   CheckPlain("> quoted line\n> second", "quoted line\nsecond");
   CheckSlack("> quoted line\n> second", "> quoted line\n> second");
   CheckGeneric("> quoted", "<blockquote>quoted</blockquote>");
   EndTest();

   StartTest(_T("Markdown - block structure"));
   CheckPlain("a\n\n---\n\nb", "a\n\nb");
   CheckTelegram("a\n\n---\n\nb", "a\n\n\xE2\x80\x94\xE2\x80\x94\xE2\x80\x94\n\nb");
   CheckGeneric("a\n\n---\n\nb", "<p>a</p>\n<hr/>\n<p>b</p>");
   CheckPlain("line one\nline two", "line one\nline two");
   CheckGeneric("line one\nline two", "<p>line one<br/>\nline two</p>");
   CheckPlain("a\n\n\n\nb", "a\n\nb");
   EndTest();

   StartTest(_T("Markdown - tables"));
   const char *table =
         "| Node | Status |\n"
         "|------|--------|\n"
         "| web-1 | up |\n"
         "| db-1 | down |";
   CheckGeneric(table,
         "<table>\n"
         "<thead>\n"
         "<tr><th>Node</th><th>Status</th></tr>\n"
         "</thead>\n"
         "<tbody>\n"
         "<tr><td>web-1</td><td>up</td></tr>\n"
         "<tr><td>db-1</td><td>down</td></tr>\n"
         "</tbody>\n"
         "</table>");
   CheckPlain(table,
         "Node  | Status\n"
         "------+-------\n"
         "web-1 | up\n"
         "db-1  | down");
   CheckTelegram(table,
         "<pre>Node  | Status\n"
         "------+-------\n"
         "web-1 | up\n"
         "db-1  | down</pre>");
   CheckSlack(table,
         "```\n"
         "Node  | Status\n"
         "------+-------\n"
         "web-1 | up\n"
         "db-1  | down\n"
         "```");
   // Alignment
   CheckGeneric("a | b | c\n:---|:---:|---:\n1 | 2 | 3",
         "<table>\n"
         "<thead>\n"
         "<tr><th align=\"left\">a</th><th align=\"center\">b</th><th align=\"right\">c</th></tr>\n"
         "</thead>\n"
         "<tbody>\n"
         "<tr><td align=\"left\">1</td><td align=\"center\">2</td><td align=\"right\">3</td></tr>\n"
         "</tbody>\n"
         "</table>");
   CheckPlain("value | count\n---:|:---\nx | 1\nlonger | 22",
         " value | count\n-------+------\n     x | 1\nlonger | 22");
   // Inline markup inside cells
   CheckGeneric("| a | b |\n|---|---|\n| **x** | `y` |",
         "<table>\n"
         "<thead>\n"
         "<tr><th>a</th><th>b</th></tr>\n"
         "</thead>\n"
         "<tbody>\n"
         "<tr><td><b>x</b></td><td><code>y</code></td></tr>\n"
         "</tbody>\n"
         "</table>");
   CheckTelegram("| a | b |\n|---|---|\n| **x** | 5 < 6 |",
         "<pre>a | b\n--+------\nx | 5 &lt; 6</pre>");   // no markup inside Telegram <pre> block
   // Missing and extra cells, escaped pipes
   CheckPlain("a | b\n---|---\n1 |\n2 | 3 | 4", "a | b\n--+--\n1 |\n2 | 3");
   CheckPlain("a | b\n---|---\nx \\| y | z", "a     | b\n------+--\nx | y | z");
   // Table surrounded by other blocks
   CheckGeneric("Text\n\n| a |\n| - |\n| 1 |\n\nMore",
         "<p>Text</p>\n<table>\n<thead>\n<tr><th>a</th></tr>\n</thead>\n<tbody>\n<tr><td>1</td></tr>\n</tbody>\n</table>\n<p>More</p>");
   CheckPlain("Text\n\nc1 | c2\n---|---\n1 | 2\n\nMore", "Text\n\nc1 | c2\n---+---\n1  | 2\n\nMore");
   // Not tables
   CheckGeneric("a\n---\nb", "<p>a</p>\n<hr/>\n<p>b</p>");
   CheckGeneric("a | b\nc | d", "<p>a | b<br/>\nc | d</p>");
   CheckPlain("```\na | b\n---|---\n```", "a | b\n---|---");
   EndTest();

   StartTest(_T("Markdown - terminal output"));
   CheckTerminal("Hello **world**!", "Hello \x1b[1mworld\x1b[22m!");
   CheckTerminal("*emph* and _also_", "\x1b[3memph\x1b[23m and \x1b[3malso\x1b[23m");
   CheckTerminal("~~gone~~", "\x1b[9mgone\x1b[29m");
   CheckTerminal("run `nxdbmgr check` now", "run \x1b[36mnxdbmgr check\x1b[39m now");
   CheckTerminal("# Title\n\nBody text", "\x1b[1m\x1b[4mTitle\x1b[24m\x1b[22m\n\nBody text");
   CheckTerminal("### Sub\n\nBody", "\x1b[1mSub\x1b[22m\n\nBody");
   CheckTerminal("See [docs](https://netxms.org) now", "See \x1b[4mdocs\x1b[24m (\x1b[36mhttps://netxms.org\x1b[39m) now");
   CheckTerminal("[https://netxms.org](https://netxms.org)", "\x1b[4mhttps://netxms.org\x1b[24m");
   CheckTerminal("```\nline1\nline2\n```", "\x1b[36m   line1\n   line2\x1b[39m");
   CheckTerminal("> quoted line\n> second", "\x1b[32m> quoted line\n> second\x1b[39m");
   CheckTerminal("- one\n- two", "\xE2\x80\xA2 one\n\xE2\x80\xA2 two");
   CheckTerminal("a | b\n---|---\n1 | 2", "\x1b[1ma\x1b[22m | \x1b[1mb\x1b[22m\n--+--\n1 | 2");
   CheckTerminal("evil \x1b[31mred", "evil [31mred");   // ESC characters in input must be dropped
   EndTest();

   StartTest(_T("Markdown - degenerate input"));
   CheckPlain("", "");
   CheckTelegram("", "");
   CheckPlain(nullptr, "");
   CheckTelegram(nullptr, "");
   CheckGeneric(nullptr, "");
   CheckSlack(nullptr, "");
   CheckTerminal(nullptr, "");
   CheckTerminal("", "");
   CheckPlain("line one\r\nline two\r\n", "line one\nline two");
   EndTest();

   StartTest(_T("Markdown - complete document"));
   const char *input =
         "# Node status\n"
         "\n"
         "Node **web-1** is *degraded*:\n"
         "\n"
         "- CPU usage > 90%\n"
         "- `httpd` not responding\n"
         "\n"
         "See [runbook](https://wiki.local/rb) for details.";
   CheckTelegram(input,
         "<b>Node status</b>\n"
         "\n"
         "Node <b>web-1</b> is <i>degraded</i>:\n"
         "\n"
         "\xE2\x80\xA2 CPU usage &gt; 90%\n"
         "\xE2\x80\xA2 <code>httpd</code> not responding\n"
         "\n"
         "See <a href=\"https://wiki.local/rb\">runbook</a> for details.");
   CheckPlain(input,
         "Node status\n"
         "\n"
         "Node web-1 is degraded:\n"
         "\n"
         "- CPU usage > 90%\n"
         "- httpd not responding\n"
         "\n"
         "See runbook (https://wiki.local/rb) for details.");
   CheckTerminal(input,
         "\x1b[1m\x1b[4mNode status\x1b[24m\x1b[22m\n"
         "\n"
         "Node \x1b[1mweb-1\x1b[22m is \x1b[3mdegraded\x1b[23m:\n"
         "\n"
         "\xE2\x80\xA2 CPU usage > 90%\n"
         "\xE2\x80\xA2 \x1b[36mhttpd\x1b[39m not responding\n"
         "\n"
         "See \x1b[4mrunbook\x1b[24m (\x1b[36mhttps://wiki.local/rb\x1b[39m) for details.");
   EndTest();
}
