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

#include <nxmarkdown.h>
#include <string>
#include <vector>

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

   virtual void text(const char *s, size_t len) = 0;
   virtual void softBreak() = 0;
   virtual void paragraphStart() = 0;
   virtual void paragraphEnd() = 0;
   virtual void headingStart(int level) = 0;
   virtual void headingEnd(int level) = 0;
   virtual void listStart(bool ordered, int depth) = 0;
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
};

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
 * Base class for renderers producing line-oriented text output (plain text, Telegram HTML,
 * Slack mrkdwn). Handles block separation and list layout; subclasses provide escaping and
 * inline markup.
 */
class TextStyleRenderer : public MarkdownRenderer
{
protected:
   bool m_afterListStart;

   TextStyleRenderer()
   {
      m_afterListStart = false;
   }

   virtual void appendEscaped(const char *s, size_t len) = 0;

   // Ensure blank line separation before a new top-level block
   void blockSeparator()
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
   virtual void text(const char *s, size_t len) override
   {
      appendEscaped(s, len);
   }

   virtual void softBreak() override
   {
      m_out.push_back('\n');
   }

   virtual void paragraphStart() override
   {
      blockSeparator();
   }

   virtual void paragraphEnd() override
   {
   }

   virtual void listStart(bool ordered, int depth) override
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
      if (ordered)
      {
         char buffer[16];
         snprintf(buffer, sizeof(buffer), "%d. ", number);
         m_out.append(buffer);
      }
      else
      {
         m_out.append(bulletMarker());
      }
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

public:
   SlackTextRenderer() : TextStyleRenderer()
   {
      m_inQuote = false;
   }

   virtual void softBreak() override
   {
      m_out.push_back('\n');
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
 * Generic HTML renderer - full structural HTML (for Matrix formatted_body, HTML e-mail, etc.)
 */
class GenericHTMLRenderer : public MarkdownRenderer
{
private:
   bool m_codeBlockHasLang;

public:
   GenericHTMLRenderer() : MarkdownRenderer()
   {
      m_codeBlockHasLang = false;
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
   virtual void listStart(bool ordered, int depth) override
   {
      m_out.append(ordered ? "<ol>\n" : "<ul>\n");
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
         m_renderer.listStart(ordered, static_cast<int>(m_lists.size()));
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
 * Parse markdown document
 */
void MarkdownParser::parse(const char *input)
{
   const char *p = input;
   while(*p != 0)
   {
      const char *eol = strchr(p, '\n');
      if (eol == nullptr)
      {
         processLine(p, strlen(p));
         break;
      }
      processLine(p, eol - p);
      p = eol + 1;
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
char LIBNXSRV_EXPORTABLE *MarkdownToPlainText(const char *markdown)
{
   PlainTextRenderer renderer;
   return Convert(markdown, renderer);
}

/**
 * Convert markdown to HTML in given dialect
 */
char LIBNXSRV_EXPORTABLE *MarkdownToHTML(const char *markdown, MarkdownHTMLDialect dialect)
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
char LIBNXSRV_EXPORTABLE *MarkdownToSlackText(const char *markdown)
{
   SlackTextRenderer renderer;
   return Convert(markdown, renderer);
}
