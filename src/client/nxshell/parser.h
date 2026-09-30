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
** File: parser.h
**
**/

#ifndef _nxshell_parser_h_
#define _nxshell_parser_h_

#include <nms_util.h>
#include <string>
#include <vector>

/**
 * Shell input mode - defines where input lines are sent by default
 */
enum class ShellMode
{
   SHELL,
   AI,
   CONSOLE,
   NXSL
};

/**
 * Target of input line
 */
enum class InputTarget
{
   NONE,       // Empty line or comment
   COMMAND,    // Shell command (builtin or alias)
   AI,         // Message for AI assistant
   CONSOLE,    // Server debug console command
   NXSL        // NXSL script
};

/**
 * Classified input line
 */
struct InputLine
{
   InputTarget target;
   std::string text;
};

/**
 * Type of "alias" command
 */
enum class AliasCommandType
{
   LIST,       // List all aliases
   SHOW,       // Show alias with given name
   DEFINE,     // Define alias
   INVALID
};

/**
 * Parsed object path
 */
struct ObjectPath
{
   bool absolute;
   std::vector<std::string> elements;
};

void TrimString(std::string *text);
void ToLowerCase(std::string *text);

InputLine ClassifyInput(ShellMode mode, const std::string& input);
void SplitCommand(const std::string& line, std::string *command, std::string *arguments);
bool SplitArguments(const std::string& text, std::vector<std::string> *arguments);

bool IsScriptComplete(const std::string& source);
bool IsInputComplete(ShellMode mode, const std::string& input);

bool IsValidAliasName(const std::string& name);
AliasCommandType ParseAliasCommand(const std::string& arguments, std::string *name, std::string *body);

void ParseObjectPath(const std::string& text, ObjectPath *path);
std::string EscapePathElement(const std::string& name);
void SplitPathForCompletion(const std::string& text, std::string *directory, std::string *prefix);

std::string StripTerminalSequences(const std::string& text);

#endif   /* _nxshell_parser_h_ */
