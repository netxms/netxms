/*
** NetXMS - Network Management System
** NetXMS shell
** Copyright (C) 2025-2026 Raden Solutions
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
** File: parser.cpp
**
**/

#include "parser.h"

/**
 * Remove leading and trailing whitespace characters
 */
void TrimString(std::string *text)
{
   size_t start = text->find_first_not_of(" \t\r\n");
   if (start == std::string::npos)
   {
      text->clear();
      return;
   }
   *text = text->substr(start, text->find_last_not_of(" \t\r\n") - start + 1);
}

/**
 * Convert ASCII characters in given string to lower case
 */
void ToLowerCase(std::string *text)
{
   for(size_t i = 0; i < text->length(); i++)
   {
      char ch = (*text)[i];
      if ((ch >= 'A') && (ch <= 'Z'))
         (*text)[i] = ch + ('a' - 'A');
   }
}

/**
 * Check if given character is an ASCII letter
 */
static inline bool IsLetter(char ch)
{
   return ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z'));
}

/**
 * Split command line into command name (converted to lower case) and arguments
 */
void SplitCommand(const std::string& line, std::string *command, std::string *arguments)
{
   size_t separator = line.find_first_of(" \t\r\n");
   if (separator != std::string::npos)
   {
      *command = line.substr(0, separator);
      *arguments = line.substr(separator + 1);
      TrimString(arguments);
   }
   else
   {
      *command = line;
      arguments->clear();
   }
   ToLowerCase(command);
}

/**
 * Get input target selected by mode command ("ai", "console", "nxsl"). Returns InputTarget::NONE
 * if given command is not a mode command.
 */
static InputTarget TargetFromModeCommand(const std::string& command)
{
   if (command == "ai")
      return InputTarget::AI;
   if ((command == "console") || (command == "con"))
      return InputTarget::CONSOLE;
   if (command == "nxsl")
      return InputTarget::NXSL;
   return InputTarget::NONE;
}

/**
 * Classify input entered in shell mode
 */
static InputLine ClassifyShellInput(const std::string& line)
{
   InputLine result;

   if (line[0] == '=')
   {
      // Expression shortcut
      std::string expression = line.substr(1);
      TrimString(&expression);
      while(!expression.empty() && (expression.back() == ';'))
         expression.pop_back();
      result.target = InputTarget::NXSL;
      result.text = "return (" + expression + ");";
      return result;
   }

   std::string command, arguments;
   SplitCommand(line, &command, &arguments);
   InputTarget target = TargetFromModeCommand(command);
   if ((target != InputTarget::NONE) && !arguments.empty())
   {
      // Mode command with text is a one-shot request to that target
      result.target = target;
      result.text = arguments;
   }
   else
   {
      result.target = InputTarget::COMMAND;
      result.text = line;
   }
   return result;
}

/**
 * Classify input according to current mode. In shell mode input is a command, an expression
 * (starts with "="), or a comment (starts with "#"). In other modes input is sent to mode's target
 * unless it starts with "/" followed by a letter or "=", which makes it a shell mode input.
 */
InputLine ClassifyInput(ShellMode mode, const std::string& input)
{
   InputLine result;
   result.text = input;
   TrimString(&result.text);

   if (result.text.empty())
   {
      result.target = InputTarget::NONE;
      return result;
   }

   if (mode == ShellMode::SHELL)
   {
      if (result.text[0] == '#')
      {
         result.target = InputTarget::NONE;
         result.text.clear();
         return result;
      }
      return ClassifyShellInput(result.text);
   }

   if ((result.text[0] == '/') && (result.text.length() > 1) && (IsLetter(result.text[1]) || (result.text[1] == '=')))
      return ClassifyShellInput(result.text.substr(1));

   switch(mode)
   {
      case ShellMode::AI:
         result.target = InputTarget::AI;
         break;
      case ShellMode::CONSOLE:
         result.target = InputTarget::CONSOLE;
         break;
      default:
         result.target = InputTarget::NXSL;
         break;
   }
   return result;
}

/**
 * Split text into arguments. Arguments are separated by whitespace; double and single quotes group
 * characters into one argument, backslash outside of single quotes makes next character literal.
 * Returns false if text contains unterminated quote.
 */
bool SplitArguments(const std::string& text, std::vector<std::string> *arguments)
{
   arguments->clear();

   std::string current;
   bool inArgument = false;
   char quote = 0;
   for(size_t i = 0; i < text.length(); i++)
   {
      char ch = text[i];
      if (quote != 0)
      {
         if (ch == quote)
            quote = 0;
         else if ((ch == '\\') && (quote == '"') && (i + 1 < text.length()))
            current.push_back(text[++i]);
         else
            current.push_back(ch);
      }
      else if ((ch == '"') || (ch == '\''))
      {
         quote = ch;
         inArgument = true;
      }
      else if ((ch == '\\') && (i + 1 < text.length()))
      {
         current.push_back(text[++i]);
         inArgument = true;
      }
      else if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n'))
      {
         if (inArgument)
         {
            arguments->push_back(current);
            current.clear();
            inArgument = false;
         }
      }
      else
      {
         current.push_back(ch);
         inArgument = true;
      }
   }

   if (quote != 0)
      return false;

   if (inArgument)
      arguments->push_back(current);
   return true;
}

/**
 * Check if NXSL source is complete, that is, all brackets, block comments, and text blocks are closed.
 * String constants and comments are skipped according to NXSL lexical rules. Source with syntax errors
 * that cannot be fixed by adding more lines is considered complete.
 */
bool IsScriptComplete(const std::string& source)
{
   int depth = 0;
   size_t length = source.length();
   size_t i = 0;
   while(i < length)
   {
      char ch = source[i];
      if ((ch == '/') && (i + 1 < length) && (source[i + 1] == '/'))
      {
         // Line comment
         size_t eol = source.find('\n', i);
         if (eol == std::string::npos)
            break;
         i = eol + 1;
      }
      else if ((ch == '/') && (i + 1 < length) && (source[i + 1] == '*'))
      {
         // Block comment, can be nested
         int level = 1;
         i += 2;
         while((i < length) && (level > 0))
         {
            if ((source[i] == '/') && (i + 1 < length) && (source[i + 1] == '*'))
            {
               level++;
               i += 2;
            }
            else if ((source[i] == '*') && (i + 1 < length) && (source[i + 1] == '/'))
            {
               level--;
               i += 2;
            }
            else
            {
               i++;
            }
         }
         if (level > 0)
            return false;
      }
      else if ((ch == '"') && (source.compare(i, 3, "\"\"\"") == 0))
      {
         // Text block
         size_t end = source.find("\"\"\"", i + 3);
         if (end == std::string::npos)
            return false;
         i = end + 3;
      }
      else if (ch == '"')
      {
         // String with escape sequences, cannot span multiple lines
         i++;
         while((i < length) && (source[i] != '"') && (source[i] != '\n'))
            i += ((source[i] == '\\') && (i + 1 < length)) ? 2 : 1;
         if ((i >= length) || (source[i] != '"'))
            return true;   // Unterminated string will be reported by compiler
         i++;
      }
      else if (ch == '\'')
      {
         // String without escape sequences, cannot span multiple lines
         i++;
         while((i < length) && (source[i] != '\'') && (source[i] != '\n'))
            i++;
         if ((i >= length) || (source[i] != '\''))
            return true;   // Unterminated string will be reported by compiler
         i++;
      }
      else
      {
         if ((ch == '(') || (ch == '{') || (ch == '['))
            depth++;
         else if ((ch == ')') || (ch == '}') || (ch == ']'))
            depth--;
         i++;
      }
   }
   return depth <= 0;
}

/**
 * Check if input is complete or more lines should be read. Only input that contains NXSL code
 * (script or alias definition) can span multiple lines.
 */
bool IsInputComplete(ShellMode mode, const std::string& input)
{
   InputLine line = ClassifyInput(mode, input);
   if (line.target == InputTarget::NXSL)
      return IsScriptComplete(line.text);

   if (line.target == InputTarget::COMMAND)
   {
      std::string command, arguments;
      SplitCommand(line.text, &command, &arguments);
      if (command == "alias")
         return IsScriptComplete(arguments);
   }
   return true;
}

/**
 * Check if given name can be used as alias name
 */
bool IsValidAliasName(const std::string& name)
{
   if (name.empty() || (!IsLetter(name[0]) && (name[0] != '_')))
      return false;
   for(size_t i = 1; i < name.length(); i++)
   {
      char ch = name[i];
      if (!IsLetter(ch) && ((ch < '0') || (ch > '9')) && (ch != '_') && (ch != '-'))
         return false;
   }
   return true;
}

/**
 * Parse arguments of "alias" command. Supported forms:
 *    (empty)           list all aliases
 *    name              show alias
 *    name = code       define alias, code is the rest of input
 *    name { code }     define alias, code is a block
 * Alias name is converted to lower case.
 */
AliasCommandType ParseAliasCommand(const std::string& arguments, std::string *name, std::string *body)
{
   name->clear();
   body->clear();

   std::string text(arguments);
   TrimString(&text);
   if (text.empty())
      return AliasCommandType::LIST;

   size_t nameEnd = text.find_first_of(" \t\r\n={");
   *name = (nameEnd != std::string::npos) ? text.substr(0, nameEnd) : text;
   ToLowerCase(name);
   if (!IsValidAliasName(*name))
      return AliasCommandType::INVALID;

   if (nameEnd == std::string::npos)
      return AliasCommandType::SHOW;

   std::string definition = text.substr(nameEnd);
   TrimString(&definition);
   if (definition.empty())
      return AliasCommandType::SHOW;

   if (definition[0] == '=')
   {
      *body = definition.substr(1);
   }
   else if ((definition[0] == '{') && (definition.back() == '}') && (definition.length() >= 2))
   {
      *body = definition.substr(1, definition.length() - 2);
   }
   else
   {
      return AliasCommandType::INVALID;
   }

   TrimString(body);
   return body->empty() ? AliasCommandType::INVALID : AliasCommandType::DEFINE;
}

/**
 * Parse object path. Path elements are separated by "/", backslash makes next character literal
 * (so that "/" within object name is written as "\/"). Path enclosed in double quotes is unquoted.
 */
void ParseObjectPath(const std::string& text, ObjectPath *path)
{
   path->absolute = false;
   path->elements.clear();

   std::string source(text);
   TrimString(&source);
   if ((source.length() >= 2) && (source[0] == '"') && (source.back() == '"'))
      source = source.substr(1, source.length() - 2);

   if (source.empty())
      return;

   path->absolute = (source[0] == '/');

   std::string current;
   for(size_t i = 0; i < source.length(); i++)
   {
      char ch = source[i];
      if ((ch == '\\') && (i + 1 < source.length()))
      {
         current.push_back(source[++i]);
      }
      else if (ch == '/')
      {
         if (!current.empty())
         {
            path->elements.push_back(current);
            current.clear();
         }
      }
      else
      {
         current.push_back(ch);
      }
   }
   if (!current.empty())
      path->elements.push_back(current);
}

/**
 * Escape object name for use as path element
 */
std::string EscapePathElement(const std::string& name)
{
   std::string result;
   for(size_t i = 0; i < name.length(); i++)
   {
      if ((name[i] == '/') || (name[i] == '\\'))
         result.push_back('\\');
      result.push_back(name[i]);
   }
   return result;
}

/**
 * Split partially entered path into directory part (everything up to and including last path
 * separator, still escaped) and unescaped prefix of the name being entered.
 */
void SplitPathForCompletion(const std::string& text, std::string *directory, std::string *prefix)
{
   size_t nameStart = 0;
   for(size_t i = 0; i < text.length(); i++)
   {
      if ((text[i] == '\\') && (i + 1 < text.length()))
         i++;
      else if (text[i] == '/')
         nameStart = i + 1;
   }

   *directory = text.substr(0, nameStart);

   prefix->clear();
   for(size_t i = nameStart; i < text.length(); i++)
   {
      if ((text[i] == '\\') && (i + 1 < text.length()))
         i++;
      prefix->push_back(text[i]);
   }
}

/**
 * Remove terminal control sequences from text
 */
std::string StripTerminalSequences(const std::string& text)
{
   std::string result;
   size_t i = 0;
   while(i < text.length())
   {
      if (text[i] != '\x1b')
      {
         result.push_back(text[i++]);
         continue;
      }

      i++;
      if ((i < text.length()) && (text[i] == '['))
      {
         // CSI sequence ends with character in range 0x40 - 0x7E
         i++;
         while((i < text.length()) && ((text[i] < 0x40) || (text[i] > 0x7E)))
            i++;
      }
      i++;
   }
   return result;
}
