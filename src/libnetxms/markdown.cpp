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
** File: markdown.cpp
**
**/

#include "libnetxms.h"
#include <nxmarkdown.h>
#include <string>
#include <vector>

/**
 * Table cell alignment as defined by delimiter row
 */
enum class TableCellAlignment
{
   DEFAULT = 0,
   LEFT = 1,
   CENTER = 2,
   RIGHT = 3
};

/**
 * Abstract renderer interface driven by the parser. Output is accumulated in m_out as UTF-8.
 */
class MarkdownRenderer
{
protected:
   std::string m_out;

public:
   virtual ~MarkdownRenderer() { }

   const std::string& output() const { return m_out; }

   /**
    * Returns true if renderer expects table cell content without inline markup (renderers that
    * lay out tables as preformatted text, where markup would either be shown literally or break
    * the surrounding preformatted block).
    */
   virtual bool plainTableCells() const { return false; }

   virtual void text(const char *s, size_t len) = 0;
   virtual void softBreak() = 0;
   virtual void paragraphStart() = 0;
   virtual void paragraphEnd() = 0;
   virtual void headingStart(int level) = 0;
   virtual void headingEnd(int level) = 0;
   virtual void listStart(bool ordered, int firstNumber, int depth) = 0;
   virtual void listEnd(bool ordered, int depth) = 0;
   virtual void listItemStart(bool ordered, int number, int depth) = 0;
   virtual void listItemEnd(int depth) = 0;
   virtual void blockquoteStart() = 0;
   virtual void blockquoteEnd() = 0;
   virtual void codeBlockStart(const char *lang, size_t langLen) = 0;
   virtual void codeBlockLine(const char *s, size_t len) = 0;
   virtual void codeBlockEnd() = 0;
   virtual void horizontalRule() = 0;
   virtual void boldStart() = 0;
   virtual void boldEnd() = 0;
   virtual void italicStart() = 0;
   virtual void italicEnd() = 0;
   virtual void strikeStart() = 0;
   virtual void strikeEnd() = 0;
   virtual void codeSpan(const char *s, size_t len) = 0;
   virtual void linkStart(const char *url, size_t urlLen) = 0;
   virtual void linkEnd(const char *url, size_t urlLen, bool urlSameAsText) = 0;
   virtual void tableStart(int columns) = 0;
   virtual void tableEnd() = 0;
   virtual void tableRowStart(bool header) = 0;
   virtual void tableRowEnd(bool header) = 0;
   virtual void tableCellStart(bool header, int column, TableCellAlignment alignment) = 0;
   virtual void tableCellEnd(bool header, int column) = 0;
};

/**
 * Count characters in UTF-8 string (continuation bytes are not counted)
 */
static size_t Utf8CharCount(const char *s, size_t len)
{
   size_t count = 0;
   for(size_t i = 0; i < len; i++)
      if ((s[i] & 0xC0) != 0x80)
         count++;
   return count;
}

/**
 * Append HTML-escaped text (&, <, >)
 */
static void AppendHTMLEscaped(std::string& out, const char *s, size_t len)
{
   for(size_t i = 0; i < len; i++)
   {
      switch(s[i])
      {
         case '&':
            out.append("&amp;");
            break;
         case '<':
            out.append("&lt;");
            break;
         case '>':
            out.append("&gt;");
            break;
         default:
            out.push_back(s[i]);
            break;
      }
   }
}

/**
 * Append HTML attribute value escaped text (&, <, >, ")
 */
static void AppendHTMLAttributeEscaped(std::string& out, const char *s, size_t len)
{
   for(size_t i = 0; i < len; i++)
   {
      switch(s[i])
      {
         case '&':
            out.append("&amp;");
            break;
         case '<':
            out.append("&lt;");
            break;
         case '>':
            out.append("&gt;");
            break;
         case '"':
            out.append("&quot;");
            break;
         default:
            out.push_back(s[i]);
            break;
      }
   }
}

/**
 * Rendered table cell for text-style renderers
 */
struct TextTableCell
{
   std::string text;
   size_t width;
};

/**
 * Base class for renderers producing line-oriented text output (plain text, Telegram HTML,
 * Slack mrkdwn). Handles block separation, list layout, and tables rendered as text with
 * aligned columns; subclasses provide escaping and inline markup.
 */
class TextStyleRenderer : public MarkdownRenderer
{
private:
   std::vector<std::vector<TextTableCell>> m_tableRows;
   std::vector<TableCellAlignment> m_tableAlignments;
   size_t m_cellOutputStart;
   size_t m_cellWidth;
   bool m_inTableCell;

   void appendPaddedCell(const TextTableCell& cell, size_t width, TableCellAlignment alignment, bool lastColumn)
   {
      size_t pad = (width > cell.width) ? width - cell.width : 0;
      size_t padBefore, padAfter;
      switch(alignment)
      {
         case TableCellAlignment::RIGHT:
            padBefore = pad;
            padAfter = 0;
            break;
         case TableCellAlignment::CENTER:
            padBefore = pad / 2;
            padAfter = pad - padBefore;
            break;
         default:
            padBefore = 0;
            padAfter = pad;
            break;
      }
      m_out.append(padBefore, ' ');
      m_out.append(cell.text);
      if (!lastColumn)
         m_out.append(padAfter, ' ');
   }

protected:
   bool m_afterListStart;
   size_t m_listIndent;   // Indent for continuation lines of current list item

   TextStyleRenderer()
   {
      m_afterListStart = false;
      m_listIndent = 0;
      m_cellOutputStart = 0;
      m_cellWidth = 0;
      m_inTableCell = false;
   }

   virtual void appendEscaped(const char *s, size_t len) = 0;

   // Markup placed around table rendered as preformatted text
   virtual const char *tablePrefix() const
   {
      return nullptr;
   }

   virtual const char *tableSuffix() const
   {
      return nullptr;
   }

   // Ensure blank line separation before a new top-level block
   void blockSeparator()
   {
      m_listIndent = 0;
      if (m_out.empty())
         return;
      size_t n = m_out.find_last_not_of('\n');
      if (n == std::string::npos)
      {
         m_out.clear();
         return;
      }
      m_out.resize(n + 1);
      m_out.append("\n\n");
   }

   // Ensure single line separation (between list items)
   void lineSeparator()
   {
      if (m_out.empty())
         return;
      size_t n = m_out.find_last_not_of('\n');
      if (n == std::string::npos)
      {
         m_out.clear();
         return;
      }
      m_out.resize(n + 1);
      m_out.push_back('\n');
   }

   virtual const char *bulletMarker() const
   {
      return "\xE2\x80\xA2 "; // "• "
   }

public:
   virtual bool plainTableCells() const override
   {
      return true;
   }

   virtual void text(const char *s, size_t len) override
   {
      if (m_inTableCell)
         m_cellWidth += Utf8CharCount(s, len);
      appendEscaped(s, len);
   }

   virtual void softBreak() override
   {
      m_out.push_back('\n');
      m_out.append(m_listIndent, ' ');
   }

   virtual void paragraphStart() override
   {
      blockSeparator();
   }

   virtual void paragraphEnd() override
   {
   }

   virtual void listStart(bool ordered, int firstNumber, int depth) override
   {
      if (depth == 0)
         blockSeparator();
      else
         lineSeparator();
      m_afterListStart = true;
   }

   virtual void listEnd(bool ordered, int depth) override
   {
   }

   virtual void listItemStart(bool ordered, int number, int depth) override
   {
      if (!m_afterListStart)
         lineSeparator();
      m_afterListStart = false;
      for(int i = 0; i < depth; i++)
         m_out.append("   ");
      size_t markerWidth;
      if (ordered)
      {
         char buffer[16];
         snprintf(buffer, sizeof(buffer), "%d. ", number);
         m_out.append(buffer);
         markerWidth = strlen(buffer);
      }
      else
      {
         const char *marker = bulletMarker();
         size_t markerLen = strlen(marker);
         m_out.append(marker, markerLen);
         markerWidth = Utf8CharCount(marker, markerLen);
      }
      m_listIndent = static_cast<size_t>(depth) * 3 + markerWidth;
   }

   virtual void listItemEnd(int depth) override
   {
   }

   virtual void blockquoteStart() override
   {
      blockSeparator();
   }

   virtual void blockquoteEnd() override
   {
   }

   virtual void horizontalRule() override
   {
      blockSeparator();
      m_out.append("\xE2\x80\x94\xE2\x80\x94\xE2\x80\x94"); // "———"
   }

   virtual void tableStart(int columns) override
   {
      blockSeparator();
      m_tableRows.clear();
      m_tableAlignments.assign(columns, TableCellAlignment::DEFAULT);
   }

   virtual void tableEnd() override
   {
      size_t columns = m_tableAlignments.size();
      std::vector<size_t> widths(columns, 0);
      for(const std::vector<TextTableCell>& row : m_tableRows)
         for(size_t i = 0; i < row.size(); i++)
            if (row[i].width > widths[i])
               widths[i] = row[i].width;

      const char *prefix = tablePrefix();
      if (prefix != nullptr)
         m_out.append(prefix);
      for(size_t r = 0; r < m_tableRows.size(); r++)
      {
         if (r > 0)
            m_out.push_back('\n');
         const std::vector<TextTableCell>& row = m_tableRows[r];
         for(size_t c = 0; c < columns; c++)
         {
            if (c > 0)
               m_out.append(" | ");
            appendPaddedCell(row[c], widths[c], m_tableAlignments[c], c == columns - 1);
         }
         while(!m_out.empty() && (m_out[m_out.length() - 1] == ' '))   // empty cell at end of row
            m_out.resize(m_out.length() - 1);
         if (r == 0)   // separator line below header row
         {
            m_out.push_back('\n');
            for(size_t c = 0; c < columns; c++)
            {
               if (c > 0)
                  m_out.append("-+-");
               m_out.append(widths[c], '-');
            }
         }
      }
      const char *suffix = tableSuffix();
      if (suffix != nullptr)
         m_out.append(suffix);

      m_tableRows.clear();
      m_tableAlignments.clear();
   }

   virtual void tableRowStart(bool header) override
   {
      m_tableRows.push_back(std::vector<TextTableCell>());
   }

   virtual void tableRowEnd(bool header) override
   {
   }

   virtual void tableCellStart(bool header, int column, TableCellAlignment alignment) override
   {
      if (header && (static_cast<size_t>(column) < m_tableAlignments.size()))
         m_tableAlignments[column] = alignment;
      m_cellOutputStart = m_out.length();
      m_cellWidth = 0;
      m_inTableCell = true;
   }

   virtual void tableCellEnd(bool header, int column) override
   {
      TextTableCell cell;
      cell.text = m_out.substr(m_cellOutputStart);
      cell.width = m_cellWidth;
      m_out.resize(m_cellOutputStart);
      m_tableRows.back().push_back(cell);
      m_inTableCell = false;
   }
};

/**
 * Plain text renderer - strips all markup
 */
class PlainTextRenderer : public TextStyleRenderer
{
protected:
   virtual void appendEscaped(const char *s, size_t len) override
   {
      m_out.append(s, len);
   }

   virtual const char *bulletMarker() const override
   {
      return "- ";
   }

public:
   virtual void headingStart(int level) override { blockSeparator(); }
   virtual void headingEnd(int level) override { }
   virtual void codeBlockStart(const char *lang, size_t langLen) override { blockSeparator(); }
   virtual void codeBlockLine(const char *s, size_t len) override
   {
      m_out.append(s, len);
      m_out.push_back('\n');
   }
   virtual void codeBlockEnd() override { }
   virtual void horizontalRule() override { blockSeparator(); }
   virtual void boldStart() override { }
   virtual void boldEnd() override { }
   virtual void italicStart() override { }
   virtual void italicEnd() override { }
   virtual void strikeStart() override { }
   virtual void strikeEnd() override { }
   virtual void codeSpan(const char *s, size_t len) override
   {
      m_out.append(s, len);
   }
   virtual void linkStart(const char *url, size_t urlLen) override { }
   virtual void linkEnd(const char *url, size_t urlLen, bool urlSameAsText) override
   {
      if (!urlSameAsText && (urlLen > 0))
      {
         m_out.append(" (");
         m_out.append(url, urlLen);
         m_out.push_back(')');
      }
   }
};

/**
 * Telegram HTML renderer - Telegram bot API tag subset with text-style layout for
 * structural elements not supported by Telegram (headings, lists, rules)
 */
class TelegramHTMLRenderer : public TextStyleRenderer
{
protected:
   virtual void appendEscaped(const char *s, size_t len) override
   {
      AppendHTMLEscaped(m_out, s, len);
   }

   virtual const char *tablePrefix() const override
   {
      return "<pre>";
   }

   virtual const char *tableSuffix() const override
   {
      return "</pre>";
   }

public:
   virtual void headingStart(int level) override
   {
      blockSeparator();
      m_out.append("<b>");
   }
   virtual void headingEnd(int level) override
   {
      m_out.append("</b>");
   }
   virtual void blockquoteStart() override
   {
      blockSeparator();
      m_out.append("<blockquote>");
   }
   virtual void blockquoteEnd() override
   {
      m_out.append("</blockquote>");
   }
   virtual void codeBlockStart(const char *lang, size_t langLen) override
   {
      blockSeparator();
      m_codeBlockHasLang = (langLen > 0);
      if (m_codeBlockHasLang)
      {
         m_out.append("<pre><code class=\"language-");
         AppendHTMLAttributeEscaped(m_out, lang, langLen);
         m_out.append("\">");
      }
      else
      {
         m_out.append("<pre>");
      }
   }
   virtual void codeBlockLine(const char *s, size_t len) override
   {
      AppendHTMLEscaped(m_out, s, len);
      m_out.push_back('\n');
   }
   virtual void codeBlockEnd() override
   {
      // Remove trailing newline of last code line to avoid empty line inside <pre>
      if (!m_out.empty() && (m_out[m_out.length() - 1] == '\n'))
         m_out.resize(m_out.length() - 1);
      m_out.append(m_codeBlockHasLang ? "</code></pre>" : "</pre>");
   }
   virtual void boldStart() override { m_out.append("<b>"); }
   virtual void boldEnd() override { m_out.append("</b>"); }
   virtual void italicStart() override { m_out.append("<i>"); }
   virtual void italicEnd() override { m_out.append("</i>"); }
   virtual void strikeStart() override { m_out.append("<s>"); }
   virtual void strikeEnd() override { m_out.append("</s>"); }
   virtual void codeSpan(const char *s, size_t len) override
   {
      m_out.append("<code>");
      AppendHTMLEscaped(m_out, s, len);
      m_out.append("</code>");
   }
   virtual void linkStart(const char *url, size_t urlLen) override
   {
      m_out.append("<a href=\"");
      AppendHTMLAttributeEscaped(m_out, url, urlLen);
      m_out.append("\">");
   }
   virtual void linkEnd(const char *url, size_t urlLen, bool urlSameAsText) override
   {
      m_out.append("</a>");
   }

private:
   bool m_codeBlockHasLang = false;
};

/**
 * Slack mrkdwn renderer
 */
class SlackTextRenderer : public TextStyleRenderer
{
private:
   bool m_inQuote;

protected:
   virtual void appendEscaped(const char *s, size_t len) override
   {
      AppendHTMLEscaped(m_out, s, len); // Slack requires exactly &, <, > escaped
   }

   virtual const char *tablePrefix() const override
   {
      return "```\n";
   }

   virtual const char *tableSuffix() const override
   {
      return "\n```";
   }

public:
   SlackTextRenderer() : TextStyleRenderer()
   {
      m_inQuote = false;
   }

   virtual void softBreak() override
   {
      TextStyleRenderer::softBreak();
      if (m_inQuote)
         m_out.append("> ");
   }
   virtual void headingStart(int level) override
   {
      blockSeparator();
      m_out.push_back('*');
   }
   virtual void headingEnd(int level) override
   {
      m_out.push_back('*');
   }
   virtual void blockquoteStart() override
   {
      blockSeparator();
      m_out.append("> ");
      m_inQuote = true;
   }
   virtual void blockquoteEnd() override
   {
      m_inQuote = false;
   }
   virtual void codeBlockStart(const char *lang, size_t langLen) override
   {
      blockSeparator();
      m_out.append("```\n");
   }
   virtual void codeBlockLine(const char *s, size_t len) override
   {
      AppendHTMLEscaped(m_out, s, len);
      m_out.push_back('\n');
   }
   virtual void codeBlockEnd() override
   {
      m_out.append("```");
   }
   virtual void boldStart() override { m_out.push_back('*'); }
   virtual void boldEnd() override { m_out.push_back('*'); }
   virtual void italicStart() override { m_out.push_back('_'); }
   virtual void italicEnd() override { m_out.push_back('_'); }
   virtual void strikeStart() override { m_out.push_back('~'); }
   virtual void strikeEnd() override { m_out.push_back('~'); }
   virtual void codeSpan(const char *s, size_t len) override
   {
      m_out.push_back('`');
      AppendHTMLEscaped(m_out, s, len);
      m_out.push_back('`');
   }
   virtual void linkStart(const char *url, size_t urlLen) override
   {
      m_out.push_back('<');
      AppendHTMLEscaped(m_out, url, urlLen);
      m_out.push_back('|');
   }
   virtual void linkEnd(const char *url, size_t urlLen, bool urlSameAsText) override
   {
      m_out.push_back('>');
   }
};

/**
 * ANSI SGR escape sequences used by terminal renderer. Styles not representable as console
 * attributes on Windows (italic, underline, strikethrough) are silently ignored by
 * WriteToTerminal there; color and bold work on all supported platforms.
 */
#define SGR_BOLD_ON        "\x1b[1m"
#define SGR_BOLD_OFF       "\x1b[22m"
#define SGR_ITALIC_ON      "\x1b[3m"
#define SGR_ITALIC_OFF     "\x1b[23m"
#define SGR_UNDERLINE_ON   "\x1b[4m"
#define SGR_UNDERLINE_OFF  "\x1b[24m"
#define SGR_STRIKE_ON      "\x1b[9m"
#define SGR_STRIKE_OFF     "\x1b[29m"
#define SGR_GREEN          "\x1b[32m"
#define SGR_CYAN           "\x1b[36m"
#define SGR_DEFAULT_FG     "\x1b[39m"

/**
 * Terminal renderer - text styled with ANSI SGR sequences for output via WriteToTerminal
 */
class TerminalRenderer : public TextStyleRenderer
{
private:
   bool m_inQuote;

protected:
   virtual void appendEscaped(const char *s, size_t len) override
   {
      // Drop ESC characters so markdown input cannot inject its own escape sequences
      for(size_t i = 0; i < len; i++)
         if (s[i] != 0x1B)
            m_out.push_back(s[i]);
   }

public:
   TerminalRenderer() : TextStyleRenderer()
   {
      m_inQuote = false;
   }

   virtual void softBreak() override
   {
      TextStyleRenderer::softBreak();
      if (m_inQuote)
         m_out.append("> ");
   }
   virtual void headingStart(int level) override
   {
      blockSeparator();
      m_out.append(SGR_BOLD_ON);
      if (level <= 2)
         m_out.append(SGR_UNDERLINE_ON);
   }
   virtual void headingEnd(int level) override
   {
      if (level <= 2)
         m_out.append(SGR_UNDERLINE_OFF);
      m_out.append(SGR_BOLD_OFF);
   }
   virtual void blockquoteStart() override
   {
      blockSeparator();
      m_out.append(SGR_GREEN "> ");
      m_inQuote = true;
   }
   virtual void blockquoteEnd() override
   {
      m_out.append(SGR_DEFAULT_FG);
      m_inQuote = false;
   }
   virtual void codeBlockStart(const char *lang, size_t langLen) override
   {
      blockSeparator();
      m_out.append(SGR_CYAN);
   }
   virtual void codeBlockLine(const char *s, size_t len) override
   {
      m_out.append("   ");
      appendEscaped(s, len);
      m_out.push_back('\n');
   }
   virtual void codeBlockEnd() override
   {
      // Remove trailing newline of last code line so following block separation is uniform
      if (!m_out.empty() && (m_out[m_out.length() - 1] == '\n'))
         m_out.resize(m_out.length() - 1);
      m_out.append(SGR_DEFAULT_FG);
   }
   virtual void boldStart() override { m_out.append(SGR_BOLD_ON); }
   virtual void boldEnd() override { m_out.append(SGR_BOLD_OFF); }
   virtual void italicStart() override { m_out.append(SGR_ITALIC_ON); }
   virtual void italicEnd() override { m_out.append(SGR_ITALIC_OFF); }
   virtual void strikeStart() override { m_out.append(SGR_STRIKE_ON); }
   virtual void strikeEnd() override { m_out.append(SGR_STRIKE_OFF); }
   virtual void codeSpan(const char *s, size_t len) override
   {
      m_out.append(SGR_CYAN);
      appendEscaped(s, len);
      m_out.append(SGR_DEFAULT_FG);
   }
   virtual void linkStart(const char *url, size_t urlLen) override
   {
      m_out.append(SGR_UNDERLINE_ON);
   }
   virtual void linkEnd(const char *url, size_t urlLen, bool urlSameAsText) override
   {
      m_out.append(SGR_UNDERLINE_OFF);
      if (!urlSameAsText && (urlLen > 0))
      {
         m_out.append(" (" SGR_CYAN);
         appendEscaped(url, urlLen);
         m_out.append(SGR_DEFAULT_FG ")");
      }
   }
   virtual void tableCellStart(bool header, int column, TableCellAlignment alignment) override
   {
      TextStyleRenderer::tableCellStart(header, column, alignment);
      if (header)
         m_out.append(SGR_BOLD_ON);   // Inside cell capture; does not affect padding width
   }
   virtual void tableCellEnd(bool header, int column) override
   {
      if (header)
         m_out.append(SGR_BOLD_OFF);
      TextStyleRenderer::tableCellEnd(header, column);
   }
};

/**
 * Generic HTML renderer - full structural HTML (for Matrix formatted_body, HTML e-mail, etc.)
 */
class GenericHTMLRenderer : public MarkdownRenderer
{
private:
   bool m_codeBlockHasLang;
   bool m_inTableBody;

public:
   GenericHTMLRenderer() : MarkdownRenderer()
   {
      m_codeBlockHasLang = false;
      m_inTableBody = false;
   }

   virtual void text(const char *s, size_t len) override
   {
      AppendHTMLEscaped(m_out, s, len);
   }
   virtual void softBreak() override
   {
      m_out.append("<br/>\n");
   }
   virtual void paragraphStart() override
   {
      m_out.append("<p>");
   }
   virtual void paragraphEnd() override
   {
      m_out.append("</p>\n");
   }
   virtual void headingStart(int level) override
   {
      char buffer[8];
      snprintf(buffer, sizeof(buffer), "<h%d>", level);
      m_out.append(buffer);
   }
   virtual void headingEnd(int level) override
   {
      char buffer[8];
      snprintf(buffer, sizeof(buffer), "</h%d>\n", level);
      m_out.append(buffer);
   }
   virtual void listStart(bool ordered, int firstNumber, int depth) override
   {
      if (!ordered)
      {
         m_out.append("<ul>\n");
      }
      else if (firstNumber != 1)
      {
         char buffer[32];
         snprintf(buffer, sizeof(buffer), "<ol start=\"%d\">\n", firstNumber);
         m_out.append(buffer);
      }
      else
      {
         m_out.append("<ol>\n");
      }
   }
   virtual void listEnd(bool ordered, int depth) override
   {
      m_out.append(ordered ? "</ol>\n" : "</ul>\n");
   }
   virtual void listItemStart(bool ordered, int number, int depth) override
   {
      m_out.append("<li>");
   }
   virtual void listItemEnd(int depth) override
   {
      m_out.append("</li>\n");
   }
   virtual void blockquoteStart() override
   {
      m_out.append("<blockquote>");
   }
   virtual void blockquoteEnd() override
   {
      m_out.append("</blockquote>\n");
   }
   virtual void codeBlockStart(const char *lang, size_t langLen) override
   {
      m_codeBlockHasLang = (langLen > 0);
      if (m_codeBlockHasLang)
      {
         m_out.append("<pre><code class=\"language-");
         AppendHTMLAttributeEscaped(m_out, lang, langLen);
         m_out.append("\">");
      }
      else
      {
         m_out.append("<pre><code>");
      }
   }
   virtual void codeBlockLine(const char *s, size_t len) override
   {
      AppendHTMLEscaped(m_out, s, len);
      m_out.push_back('\n');
   }
   virtual void codeBlockEnd() override
   {
      m_out.append("</code></pre>\n");
   }
   virtual void horizontalRule() override
   {
      m_out.append("<hr/>\n");
   }
   virtual void boldStart() override { m_out.append("<b>"); }
   virtual void boldEnd() override { m_out.append("</b>"); }
   virtual void italicStart() override { m_out.append("<i>"); }
   virtual void italicEnd() override { m_out.append("</i>"); }
   virtual void strikeStart() override { m_out.append("<s>"); }
   virtual void strikeEnd() override { m_out.append("</s>"); }
   virtual void codeSpan(const char *s, size_t len) override
   {
      m_out.append("<code>");
      AppendHTMLEscaped(m_out, s, len);
      m_out.append("</code>");
   }
   virtual void linkStart(const char *url, size_t urlLen) override
   {
      m_out.append("<a href=\"");
      AppendHTMLAttributeEscaped(m_out, url, urlLen);
      m_out.append("\">");
   }
   virtual void linkEnd(const char *url, size_t urlLen, bool urlSameAsText) override
   {
      m_out.append("</a>");
   }
   virtual void tableStart(int columns) override
   {
      m_out.append("<table>\n");
   }
   virtual void tableEnd() override
   {
      m_out.append(m_inTableBody ? "</tbody>\n</table>\n" : "</table>\n");
      m_inTableBody = false;
   }
   virtual void tableRowStart(bool header) override
   {
      if (header)
         m_out.append("<thead>\n");
      m_out.append("<tr>");
   }
   virtual void tableRowEnd(bool header) override
   {
      m_out.append("</tr>\n");
      if (header)
      {
         m_out.append("</thead>\n<tbody>\n");
         m_inTableBody = true;
      }
   }
   virtual void tableCellStart(bool header, int column, TableCellAlignment alignment) override
   {
      m_out.append(header ? "<th" : "<td");
      switch(alignment)
      {
         case TableCellAlignment::LEFT:
            m_out.append(" align=\"left\"");
            break;
         case TableCellAlignment::CENTER:
            m_out.append(" align=\"center\"");
            break;
         case TableCellAlignment::RIGHT:
            m_out.append(" align=\"right\"");
            break;
         default:
            break;
      }
      m_out.push_back('>');
   }
   virtual void tableCellEnd(bool header, int column) override
   {
      m_out.append(header ? "</th>" : "</td>");
   }
};

/**
 * Find sequence of given characters in text
 */
static ssize_t FindSequence(const char *s, size_t len, size_t from, const char *seq, size_t seqLen)
{
   if (seqLen > len)
      return -1;
   for(size_t i = from; i <= len - seqLen; i++)
   {
      if (!memcmp(&s[i], seq, seqLen))
         return static_cast<ssize_t>(i);
   }
   return -1;
}

/**
 * Check if character is ASCII punctuation (candidates for backslash escaping)
 */
static inline bool IsEscapablePunctuation(char c)
{
   return strchr("\\`*_{}[]()#+-.!<>~|\"'", c) != nullptr;
}

/**
 * Check if text within angle brackets is an absolute URI (scheme of at least two characters
 * followed by colon and at least one more character)
 */
static bool IsAutolinkURI(const char *s, size_t len)
{
   if (!isalpha(static_cast<unsigned char>(*s)))
      return false;
   size_t i = 1;
   while((i < len) && (isalnum(static_cast<unsigned char>(s[i])) || (s[i] == '+') || (s[i] == '.') || (s[i] == '-')))
      i++;
   return (i >= 2) && (i <= 32) && (i + 1 < len) && (s[i] == ':');
}

/**
 * Check if text within angle brackets is an e-mail address
 */
static bool IsAutolinkEmail(const char *s, size_t len)
{
   const char *at = static_cast<const char*>(memchr(s, '@', len));
   if ((at == nullptr) || (at == s) || (at == &s[len - 1]))
      return false;

   const char *domain = at + 1;
   size_t domainLen = len - (domain - s);
   if (memchr(domain, '@', domainLen) != nullptr)
      return false;

   const char *dot = static_cast<const char*>(memchr(domain, '.', domainLen));
   if ((dot == nullptr) || (dot == domain) || (dot == &s[len - 1]))
      return false;

   for(size_t i = 0; i < len; i++)
      if ((s[i] == ':') || (s[i] == '/'))
         return false;
   return true;
}

/**
 * Inline markdown parser - processes emphasis, code spans, and links within a text run
 */
static void ParseInline(const char *s, size_t len, MarkdownRenderer& renderer)
{
   size_t i = 0, textStart = 0;

   auto flushText = [&] ()
   {
      if (i > textStart)
         renderer.text(&s[textStart], i - textStart);
   };

   while(i < len)
   {
      char c = s[i];

      // Backslash escape
      if ((c == '\\') && (i + 1 < len) && IsEscapablePunctuation(s[i + 1]))
      {
         flushText();
         renderer.text(&s[i + 1], 1);
         i += 2;
         textStart = i;
         continue;
      }

      // Code span
      if (c == '`')
      {
         size_t n = 1;
         while ((i + n < len) && (s[i + n] == '`'))
            n++;
         // Find closing run of exactly n backticks
         ssize_t j = -1;
         size_t searchFrom = i + n;
         while(searchFrom < len)
         {
            ssize_t candidate = FindSequence(s, len, searchFrom, &s[i], n);
            if (candidate < 0)
               break;
            bool longerRun = (static_cast<size_t>(candidate) + n < len) && (s[candidate + n] == '`');
            if (!longerRun)
            {
               j = candidate;
               break;
            }
            // Skip the entire longer run
            searchFrom = candidate;
            while ((searchFrom < len) && (s[searchFrom] == '`'))
               searchFrom++;
         }
         if (j > static_cast<ssize_t>(i))
         {
            flushText();
            renderer.codeSpan(&s[i + n], j - i - n);
            i = j + n;
            textStart = i;
         }
         else
         {
            i += n;
         }
         continue;
      }

      // Bold / italic (asterisk)
      if (c == '*')
      {
         if ((i + 2 < len) && (s[i + 1] == '*') && (s[i + 2] == '*'))
         {
            ssize_t j = FindSequence(s, len, i + 3, "***", 3);
            if (j > static_cast<ssize_t>(i + 3))
            {
               flushText();
               renderer.boldStart();
               renderer.italicStart();
               ParseInline(&s[i + 3], j - i - 3, renderer);
               renderer.italicEnd();
               renderer.boldEnd();
               i = j + 3;
               textStart = i;
               continue;
            }
            // No triple closer - fall through to double asterisk handling
         }
         if ((i + 1 < len) && (s[i + 1] == '*'))
         {
            ssize_t j = FindSequence(s, len, i + 2, "**", 2);
            if ((j > static_cast<ssize_t>(i + 2)))
            {
               flushText();
               renderer.boldStart();
               ParseInline(&s[i + 2], j - i - 2, renderer);
               renderer.boldEnd();
               i = j + 2;
               textStart = i;
               continue;
            }
            i += 2;
            continue;
         }
         if ((i + 1 < len) && (s[i + 1] != ' ') && (s[i + 1] != '*'))
         {
            // Find closing single asterisk (not part of a double run, not preceded by space)
            ssize_t j = -1;
            for(size_t k = i + 2; k < len; k++)
            {
               if ((s[k] == '*') && (s[k - 1] != '*') && (s[k - 1] != ' ') && ((k + 1 >= len) || (s[k + 1] != '*')))
               {
                  j = static_cast<ssize_t>(k);
                  break;
               }
            }
            if (j > 0)
            {
               flushText();
               renderer.italicStart();
               ParseInline(&s[i + 1], j - i - 1, renderer);
               renderer.italicEnd();
               i = j + 1;
               textStart = i;
               continue;
            }
         }
         i++;
         continue;
      }

      // Italic (underscore) - only at word boundaries so snake_case identifiers survive
      if (c == '_')
      {
         bool openerValid = ((i == 0) || !isalnum(static_cast<unsigned char>(s[i - 1]))) &&
                            (i + 1 < len) && (s[i + 1] != ' ') && (s[i + 1] != '_');
         if (openerValid)
         {
            ssize_t j = -1;
            for(size_t k = i + 2; k < len; k++)
            {
               if ((s[k] == '_') && (s[k - 1] != ' ') && ((k + 1 >= len) || !isalnum(static_cast<unsigned char>(s[k + 1]))))
               {
                  j = static_cast<ssize_t>(k);
                  break;
               }
            }
            if (j > 0)
            {
               flushText();
               renderer.italicStart();
               ParseInline(&s[i + 1], j - i - 1, renderer);
               renderer.italicEnd();
               i = j + 1;
               textStart = i;
               continue;
            }
         }
         i++;
         continue;
      }

      // Strikethrough
      if ((c == '~') && (i + 1 < len) && (s[i + 1] == '~'))
      {
         ssize_t j = FindSequence(s, len, i + 2, "~~", 2);
         if (j > static_cast<ssize_t>(i + 2))
         {
            flushText();
            renderer.strikeStart();
            ParseInline(&s[i + 2], j - i - 2, renderer);
            renderer.strikeEnd();
            i = j + 2;
            textStart = i;
            continue;
         }
         i += 2;
         continue;
      }

      // Autolink <https://example.com> or <user@example.com>
      if (c == '<')
      {
         size_t end = i + 1;
         while((end < len) && (s[end] != '>') && (s[end] != '<') && (s[end] != ' ') && (s[end] != '\t'))
            end++;
         if ((end < len) && (s[end] == '>') && (end > i + 1))
         {
            const char *linkText = &s[i + 1];
            size_t linkTextLen = end - i - 1;
            std::string url;
            if (IsAutolinkURI(linkText, linkTextLen))
               url.assign(linkText, linkTextLen);
            else if (IsAutolinkEmail(linkText, linkTextLen))
               url.assign("mailto:").append(linkText, linkTextLen);
            if (!url.empty())
            {
               flushText();
               renderer.linkStart(url.c_str(), url.length());
               renderer.text(linkText, linkTextLen);
               renderer.linkEnd(url.c_str(), url.length(), true);   // link text already shows the target
               i = end + 1;
               textStart = i;
               continue;
            }
         }
         i++;
         continue;
      }

      // Link [text](url)
      if (c == '[')
      {
         ssize_t closeBracket = FindSequence(s, len, i + 1, "]", 1);
         if ((closeBracket > 0) && (static_cast<size_t>(closeBracket) + 1 < len) && (s[closeBracket + 1] == '('))
         {
            ssize_t closeParen = FindSequence(s, len, closeBracket + 2, ")", 1);
            if (closeParen > 0)
            {
               flushText();
               const char *linkText = &s[i + 1];
               size_t linkTextLen = closeBracket - i - 1;
               const char *url = &s[closeBracket + 2];
               size_t urlLen = closeParen - closeBracket - 2;
               bool urlSameAsText = (linkTextLen == urlLen) && !memcmp(linkText, url, urlLen);
               renderer.linkStart(url, urlLen);
               ParseInline(linkText, linkTextLen, renderer);
               renderer.linkEnd(url, urlLen, urlSameAsText);
               i = closeParen + 1;
               textStart = i;
               continue;
            }
         }
         i++;
         continue;
      }

      i++;
   }
   flushText();
}

/**
 * List nesting state
 */
struct ListLevel
{
   bool ordered;
   bool itemOpen;
};

/**
 * Trim leading and trailing whitespace
 */
static std::string TrimWhitespace(const std::string& s)
{
   size_t start = s.find_first_not_of(" \t");
   if (start == std::string::npos)
      return std::string();
   size_t end = s.find_last_not_of(" \t");
   return s.substr(start, end - start + 1);
}

/**
 * Check if line contains pipe character not escaped by backslash
 */
static bool ContainsUnescapedPipe(const char *s, size_t len)
{
   for(size_t i = 0; i < len; i++)
   {
      if (s[i] == '\\')
         i++;
      else if (s[i] == '|')
         return true;
   }
   return false;
}

/**
 * Split table row into trimmed cells. Leading and trailing pipes are optional, escaped pipes
 * are left intact for inline parser to unescape.
 */
static void SplitTableRow(const char *s, size_t len, std::vector<std::string> *cells)
{
   while((len > 0) && ((s[len - 1] == ' ') || (s[len - 1] == '\t') || (s[len - 1] == '\r')))
      len--;

   size_t start = 0;
   while((start < len) && ((s[start] == ' ') || (s[start] == '\t')))
      start++;
   if ((start < len) && (s[start] == '|'))
      start++;
   if ((len > start) && (s[len - 1] == '|') && ((len - 1 == start) || (s[len - 2] != '\\')))
      len--;

   std::string current;
   for(size_t i = start; i < len; i++)
   {
      if ((s[i] == '\\') && (i + 1 < len) && (s[i + 1] == '|'))
      {
         current.append("\\|");
         i++;
      }
      else if (s[i] == '|')
      {
         cells->push_back(TrimWhitespace(current));
         current.clear();
      }
      else
      {
         current.push_back(s[i]);
      }
   }
   cells->push_back(TrimWhitespace(current));
}

/**
 * Check if line is a table delimiter row (---|:---:|---:) and read column alignments from it
 */
static bool IsTableDelimiterRow(const char *s, size_t len, std::vector<TableCellAlignment> *alignments)
{
   if (!ContainsUnescapedPipe(s, len))
      return false;

   std::vector<std::string> cells;
   SplitTableRow(s, len, &cells);
   for(const std::string& cell : cells)
   {
      size_t i = 0;
      bool alignLeft = (i < cell.length()) && (cell[i] == ':');
      if (alignLeft)
         i++;
      size_t dashes = 0;
      while((i < cell.length()) && (cell[i] == '-'))
      {
         dashes++;
         i++;
      }
      bool alignRight = (i < cell.length()) && (cell[i] == ':');
      if (alignRight)
         i++;
      if ((dashes == 0) || (i != cell.length()))
         return false;
      alignments->push_back(alignLeft ?
            (alignRight ? TableCellAlignment::CENTER : TableCellAlignment::LEFT) :
            (alignRight ? TableCellAlignment::RIGHT : TableCellAlignment::DEFAULT));
   }
   return true;
}

/**
 * Block-level markdown parser
 */
class MarkdownParser
{
private:
   MarkdownRenderer& m_renderer;
   std::vector<ListLevel> m_lists;
   bool m_inParagraph;
   bool m_inQuote;
   bool m_inFence;
   char m_fenceChar;
   size_t m_fenceLen;

   void closeParagraph()
   {
      if (m_inParagraph)
      {
         m_renderer.paragraphEnd();
         m_inParagraph = false;
      }
   }

   void closeQuote()
   {
      if (m_inQuote)
      {
         m_renderer.blockquoteEnd();
         m_inQuote = false;
      }
   }

   void popList()
   {
      ListLevel& l = m_lists.back();
      int depth = static_cast<int>(m_lists.size()) - 1;
      if (l.itemOpen)
         m_renderer.listItemEnd(depth);
      m_renderer.listEnd(l.ordered, depth);
      m_lists.pop_back();
   }

   void closeLists()
   {
      while(!m_lists.empty())
         popList();
   }

   void closeAllBlocks()
   {
      closeParagraph();
      closeQuote();
      closeLists();
   }

   void startListItem(bool ordered, int number, int depth)
   {
      closeParagraph();
      closeQuote();
      while(static_cast<int>(m_lists.size()) > depth + 1)
         popList();
      if ((static_cast<int>(m_lists.size()) == depth + 1) && (m_lists.back().ordered != ordered))
         popList();
      while(static_cast<int>(m_lists.size()) < depth + 1)
      {
         m_renderer.listStart(ordered, number, static_cast<int>(m_lists.size()));
         ListLevel l;
         l.ordered = ordered;
         l.itemOpen = false;
         m_lists.push_back(l);
      }
      ListLevel& l = m_lists.back();
      if (l.itemOpen)
         m_renderer.listItemEnd(depth);
      m_renderer.listItemStart(ordered, number, depth);
      l.itemOpen = true;
   }

   void processLine(const char *line, size_t len);
   void renderTableCell(const std::string& content, bool header, int column, TableCellAlignment alignment);
   bool processTable(const std::vector<std::string>& lines, size_t *index);

public:
   MarkdownParser(MarkdownRenderer& renderer) : m_renderer(renderer)
   {
      m_inParagraph = false;
      m_inQuote = false;
      m_inFence = false;
      m_fenceChar = 0;
      m_fenceLen = 0;
   }

   void parse(const char *input);
};

/**
 * Check if line is a horizontal rule (at least 3 of the same marker character, spaces allowed)
 */
static bool IsHorizontalRule(const char *s, size_t len)
{
   char marker = 0;
   int count = 0;
   for(size_t i = 0; i < len; i++)
   {
      char c = s[i];
      if ((c == ' ') || (c == '\t'))
         continue;
      if ((c != '-') && (c != '*') && (c != '_'))
         return false;
      if (marker == 0)
         marker = c;
      else if (c != marker)
         return false;
      count++;
   }
   return count >= 3;
}

/**
 * Process single line of markdown input
 */
void MarkdownParser::processLine(const char *line, size_t len)
{
   // Strip trailing whitespace (also handles \r from CRLF input)
   while((len > 0) && ((line[len - 1] == ' ') || (line[len - 1] == '\t') || (line[len - 1] == '\r')))
      len--;

   if (m_inFence)
   {
      // Check for closing fence
      size_t indent = 0;
      while((indent < len) && (line[indent] == ' '))
         indent++;
      size_t run = 0;
      while((indent + run < len) && (line[indent + run] == m_fenceChar))
         run++;
      if ((indent <= 3) && (run >= m_fenceLen) && (indent + run == len))
      {
         m_renderer.codeBlockEnd();
         m_inFence = false;
      }
      else
      {
         m_renderer.codeBlockLine(line, len);
      }
      return;
   }

   // Blank line closes all open blocks
   size_t indent = 0;
   while((indent < len) && ((line[indent] == ' ') || (line[indent] == '\t')))
      indent++;
   if (indent == len)
   {
      closeAllBlocks();
      return;
   }

   const char *p = &line[indent];
   size_t rem = len - indent;

   // Fenced code block start
   if ((indent <= 3) && ((*p == '`') || (*p == '~')))
   {
      size_t run = 0;
      while((run < rem) && (p[run] == *p))
         run++;
      if (run >= 3)
      {
         // Language identifier follows the fence
         size_t langStart = run;
         while((langStart < rem) && (p[langStart] == ' '))
            langStart++;
         size_t langEnd = langStart;
         while((langEnd < rem) && (p[langEnd] != ' ') && (p[langEnd] != '`'))
            langEnd++;
         closeAllBlocks();
         m_renderer.codeBlockStart(&p[langStart], langEnd - langStart);
         m_inFence = true;
         m_fenceChar = *p;
         m_fenceLen = run;
         return;
      }
   }

   // Heading
   if ((indent <= 3) && (*p == '#'))
   {
      size_t level = 0;
      while((level < rem) && (p[level] == '#'))
         level++;
      if ((level <= 6) && ((level == rem) || (p[level] == ' ')))
      {
         size_t start = level;
         while((start < rem) && (p[start] == ' '))
            start++;
         size_t end = rem;
         // Strip optional closing sequence of #'s
         while((end > start) && (p[end - 1] == '#'))
            end--;
         while((end > start) && (p[end - 1] == ' '))
            end--;
         closeAllBlocks();
         m_renderer.headingStart(static_cast<int>(level));
         ParseInline(&p[start], end - start, m_renderer);
         m_renderer.headingEnd(static_cast<int>(level));
         return;
      }
   }

   // Horizontal rule (must be checked before bullet - "- - -" parses as either)
   if ((indent <= 3) && IsHorizontalRule(p, rem))
   {
      closeAllBlocks();
      m_renderer.horizontalRule();
      return;
   }

   // Blockquote
   if (*p == '>')
   {
      size_t start = 1;
      if ((start < rem) && (p[start] == ' '))
         start++;
      if (!m_inQuote)
      {
         closeParagraph();
         closeLists();
         m_renderer.blockquoteStart();
         m_inQuote = true;
      }
      else
      {
         m_renderer.softBreak();
      }
      ParseInline(&p[start], rem - start, m_renderer);
      return;
   }

   // Bullet list item
   if (((*p == '-') || (*p == '*') || (*p == '+')) && (rem >= 2) && (p[1] == ' '))
   {
      size_t start = 2;
      while((start < rem) && (p[start] == ' '))
         start++;
      startListItem(false, 0, (indent >= 2) ? 1 : 0);
      ParseInline(&p[start], rem - start, m_renderer);
      return;
   }

   // Ordered list item
   if (isdigit(static_cast<unsigned char>(*p)))
   {
      size_t numLen = 0;
      while((numLen < rem) && (numLen < 9) && isdigit(static_cast<unsigned char>(p[numLen])))
         numLen++;
      if ((numLen + 1 < rem) && ((p[numLen] == '.') || (p[numLen] == ')')) && (p[numLen + 1] == ' '))
      {
         int number = static_cast<int>(strtol(p, nullptr, 10));
         size_t start = numLen + 2;
         while((start < rem) && (p[start] == ' '))
            start++;
         startListItem(true, number, (indent >= 2) ? 1 : 0);
         ParseInline(&p[start], rem - start, m_renderer);
         return;
      }
   }

   // Continuation line of current list item (list items are often wrapped across lines)
   if (!m_lists.empty() && m_lists.back().itemOpen)
   {
      m_renderer.softBreak();
      ParseInline(p, rem, m_renderer);
      return;
   }

   // Regular paragraph text
   closeQuote();
   closeLists();
   if (m_inParagraph)
   {
      m_renderer.softBreak();
   }
   else
   {
      m_renderer.paragraphStart();
      m_inParagraph = true;
   }
   ParseInline(p, rem, m_renderer);
}

/**
 * Render single table cell
 */
void MarkdownParser::renderTableCell(const std::string& content, bool header, int column, TableCellAlignment alignment)
{
   m_renderer.tableCellStart(header, column, alignment);
   if (m_renderer.plainTableCells())
   {
      PlainTextRenderer plainText;
      ParseInline(content.c_str(), content.length(), plainText);
      const std::string& text = plainText.output();
      if (!text.empty())
         m_renderer.text(text.c_str(), text.length());
   }
   else
   {
      ParseInline(content.c_str(), content.length(), m_renderer);
   }
   m_renderer.tableCellEnd(header, column);
}

/**
 * Process table starting at given line. Returns false if lines at given position do not form
 * a table, otherwise renders it and moves index to the last line of the table.
 */
bool MarkdownParser::processTable(const std::vector<std::string>& lines, size_t *index)
{
   const std::string& header = lines[*index];
   if (!ContainsUnescapedPipe(header.c_str(), header.length()))
      return false;

   const std::string& delimiter = lines[*index + 1];
   std::vector<TableCellAlignment> alignments;
   if (!IsTableDelimiterRow(delimiter.c_str(), delimiter.length(), &alignments))
      return false;

   std::vector<std::string> headerCells;
   SplitTableRow(header.c_str(), header.length(), &headerCells);
   if (headerCells.size() != alignments.size())
      return false;

   closeAllBlocks();

   int columns = static_cast<int>(headerCells.size());
   m_renderer.tableStart(columns);
   m_renderer.tableRowStart(true);
   for(int i = 0; i < columns; i++)
      renderTableCell(headerCells[i], true, i, alignments[i]);
   m_renderer.tableRowEnd(true);

   // Table body ends at blank line or at line that cannot be a table row
   size_t line = *index + 2;
   while(line < lines.size())
   {
      const std::string& row = lines[line];
      if (!ContainsUnescapedPipe(row.c_str(), row.length()))
         break;

      std::vector<std::string> cells;
      SplitTableRow(row.c_str(), row.length(), &cells);
      m_renderer.tableRowStart(false);
      for(int i = 0; i < columns; i++)
         renderTableCell((static_cast<size_t>(i) < cells.size()) ? cells[i] : std::string(), false, i, alignments[i]);
      m_renderer.tableRowEnd(false);
      line++;
   }
   m_renderer.tableEnd();

   *index = line - 1;
   return true;
}

/**
 * Parse markdown document
 */
void MarkdownParser::parse(const char *input)
{
   std::vector<std::string> lines;
   const char *p = input;
   while(*p != 0)
   {
      const char *eol = strchr(p, '\n');
      if (eol == nullptr)
      {
         lines.push_back(std::string(p));
         break;
      }
      lines.push_back(std::string(p, eol - p));
      p = eol + 1;
   }

   for(size_t i = 0; i < lines.size(); i++)
   {
      if (!m_inFence && (i + 1 < lines.size()) && processTable(lines, &i))
         continue;
      processLine(lines[i].c_str(), lines[i].length());
   }

   if (m_inFence)
      m_renderer.codeBlockEnd();
   closeAllBlocks();
}

/**
 * Run conversion with given renderer
 */
static char *Convert(const char *markdown, MarkdownRenderer& renderer)
{
   if (markdown != nullptr)
   {
      MarkdownParser parser(renderer);
      parser.parse(markdown);
   }
   std::string result = renderer.output();
   size_t n = result.find_last_not_of("\n ");
   result.resize((n == std::string::npos) ? 0 : n + 1);
   return MemCopyStringA(result.c_str());
}

/**
 * Convert markdown to plain text (strip all markup)
 */
char LIBNETXMS_EXPORTABLE *MarkdownToPlainText(const char *markdown)
{
   PlainTextRenderer renderer;
   return Convert(markdown, renderer);
}

/**
 * Convert markdown to HTML in given dialect
 */
char LIBNETXMS_EXPORTABLE *MarkdownToHTML(const char *markdown, MarkdownHTMLDialect dialect)
{
   if (dialect == MarkdownHTMLDialect::TELEGRAM)
   {
      TelegramHTMLRenderer renderer;
      return Convert(markdown, renderer);
   }
   GenericHTMLRenderer renderer;
   return Convert(markdown, renderer);
}

/**
 * Convert markdown to Slack mrkdwn format
 */
char LIBNETXMS_EXPORTABLE *MarkdownToSlackText(const char *markdown)
{
   SlackTextRenderer renderer;
   return Convert(markdown, renderer);
}

/**
 * Convert markdown to text styled with ANSI SGR escape sequences for terminal output
 */
char LIBNETXMS_EXPORTABLE *MarkdownToTerminal(const char *markdown)
{
   TerminalRenderer renderer;
   return Convert(markdown, renderer);
}
