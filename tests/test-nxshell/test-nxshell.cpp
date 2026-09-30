/*
** NetXMS - Network Management System
** Unit tests for NetXMS shell input parsing
** Copyright (C) 2026 Raden Solutions
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
** File: test-nxshell.cpp
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <testtools.h>
#include <netxms-version.h>
#include <parser.h>

NETXMS_EXECUTABLE_HEADER(test-nxshell)

/**
 * Check classification of input
 */
static void AssertInput(ShellMode mode, const char *input, InputTarget target, const char *text)
{
   InputLine line = ClassifyInput(mode, input);
   AssertTrue(line.target == target);
   AssertEquals(line.text.c_str(), text);
}

/**
 * Test input classification
 */
static void TestClassifyInput()
{
   StartTest(_T("ClassifyInput"));

   // Shell mode
   AssertInput(ShellMode::SHELL, "", InputTarget::NONE, "");
   AssertInput(ShellMode::SHELL, "   \t ", InputTarget::NONE, "");
   AssertInput(ShellMode::SHELL, "# comment", InputTarget::NONE, "");
   AssertInput(ShellMode::SHELL, "  ls /Infrastructure  ", InputTarget::COMMAND, "ls /Infrastructure");
   AssertInput(ShellMode::SHELL, "cd #123", InputTarget::COMMAND, "cd #123");
   AssertInput(ShellMode::SHELL, "unknown", InputTarget::COMMAND, "unknown");
   AssertInput(ShellMode::SHELL, "/ls", InputTarget::COMMAND, "/ls");

   // Mode commands: one-shot with text, mode switch without
   AssertInput(ShellMode::SHELL, "ai why is it down?", InputTarget::AI, "why is it down?");
   AssertInput(ShellMode::SHELL, "AI hello", InputTarget::AI, "hello");
   AssertInput(ShellMode::SHELL, "console show pollers", InputTarget::CONSOLE, "show pollers");
   AssertInput(ShellMode::SHELL, "con show threads", InputTarget::CONSOLE, "show threads");
   AssertInput(ShellMode::SHELL, "nxsl println(1);", InputTarget::NXSL, "println(1);");
   AssertInput(ShellMode::SHELL, "ai", InputTarget::COMMAND, "ai");
   AssertInput(ShellMode::SHELL, "console  ", InputTarget::COMMAND, "console");
   AssertInput(ShellMode::SHELL, "nxsl", InputTarget::COMMAND, "nxsl");
   AssertInput(ShellMode::SHELL, "air hello", InputTarget::COMMAND, "air hello");

   // Expression shortcut
   AssertInput(ShellMode::SHELL, "= 2 + 2", InputTarget::NXSL, "return (2 + 2);");
   AssertInput(ShellMode::SHELL, "=$node.name;", InputTarget::NXSL, "return ($node.name);");

   // Sticky modes
   AssertInput(ShellMode::AI, "why is it down?", InputTarget::AI, "why is it down?");
   AssertInput(ShellMode::AI, "# not a comment", InputTarget::AI, "# not a comment");
   AssertInput(ShellMode::AI, "ls", InputTarget::AI, "ls");
   AssertInput(ShellMode::CONSOLE, "show pollers", InputTarget::CONSOLE, "show pollers");
   AssertInput(ShellMode::CONSOLE, "exit", InputTarget::CONSOLE, "exit");
   AssertInput(ShellMode::NXSL, "println(1);", InputTarget::NXSL, "println(1);");
   AssertInput(ShellMode::NXSL, "= 1", InputTarget::NXSL, "= 1");
   AssertInput(ShellMode::AI, "   ", InputTarget::NONE, "");

   // Escape to shell from sticky modes
   AssertInput(ShellMode::AI, "/cd ..", InputTarget::COMMAND, "cd ..");
   AssertInput(ShellMode::AI, "/exit", InputTarget::COMMAND, "exit");
   AssertInput(ShellMode::AI, "/console show threads", InputTarget::CONSOLE, "show threads");
   AssertInput(ShellMode::CONSOLE, "/ai what is this?", InputTarget::AI, "what is this?");
   AssertInput(ShellMode::CONSOLE, "/= 1 + 1", InputTarget::NXSL, "return (1 + 1);");
   AssertInput(ShellMode::AI, "/nxsl", InputTarget::COMMAND, "nxsl");

   // NXSL comments and division are not shell commands
   AssertInput(ShellMode::NXSL, "// comment", InputTarget::NXSL, "// comment");
   AssertInput(ShellMode::NXSL, "/* comment */ x = 1;", InputTarget::NXSL, "/* comment */ x = 1;");
   AssertInput(ShellMode::NXSL, "/ 2", InputTarget::NXSL, "/ 2");
   AssertInput(ShellMode::AI, "/", InputTarget::AI, "/");
   AssertInput(ShellMode::AI, "/1", InputTarget::AI, "/1");

   EndTest();
}

/**
 * Test command and argument splitting
 */
static void TestSplit()
{
   StartTest(_T("SplitCommand"));

   std::string command, arguments;
   SplitCommand("CD /Infrastructure/Core", &command, &arguments);
   AssertEquals(command.c_str(), "cd");
   AssertEquals(arguments.c_str(), "/Infrastructure/Core");

   SplitCommand("ls", &command, &arguments);
   AssertEquals(command.c_str(), "ls");
   AssertEquals(arguments.c_str(), "");

   SplitCommand("alias x {\n   return 1;\n}", &command, &arguments);
   AssertEquals(command.c_str(), "alias");
   AssertEquals(arguments.c_str(), "x {\n   return 1;\n}");

   EndTest();

   StartTest(_T("SplitArguments"));

   std::vector<std::string> args;
   AssertTrue(SplitArguments("", &args));
   AssertEquals(args.size(), 0);

   AssertTrue(SplitArguments("  one   two\tthree ", &args));
   AssertEquals(args.size(), 3);
   AssertEquals(args[0].c_str(), "one");
   AssertEquals(args[1].c_str(), "two");
   AssertEquals(args[2].c_str(), "three");

   AssertTrue(SplitArguments("\"two words\" 'it\\s' a\\ b \"\" x\"y z\"", &args));
   AssertEquals(args.size(), 5);
   AssertEquals(args[0].c_str(), "two words");
   AssertEquals(args[1].c_str(), "it\\s");
   AssertEquals(args[2].c_str(), "a b");
   AssertEquals(args[3].c_str(), "");
   AssertEquals(args[4].c_str(), "xy z");

   AssertTrue(SplitArguments("\"quote \\\" inside\"", &args));
   AssertEquals(args.size(), 1);
   AssertEquals(args[0].c_str(), "quote \" inside");

   AssertFalse(SplitArguments("\"unterminated", &args));
   AssertFalse(SplitArguments("it's", &args));

   EndTest();
}

/**
 * Test detection of complete input
 */
static void TestInputCompleteness()
{
   StartTest(_T("IsScriptComplete"));

   AssertTrue(IsScriptComplete(""));
   AssertTrue(IsScriptComplete("println(1);"));
   AssertTrue(IsScriptComplete("if (x) { y = [1, 2]; }"));
   AssertFalse(IsScriptComplete("if (x) {"));
   AssertFalse(IsScriptComplete("println(1,"));
   AssertFalse(IsScriptComplete("a = [1, 2,"));
   AssertTrue(IsScriptComplete("if (x) {\n   println(1);\n}"));

   // Brackets within strings and comments are ignored
   AssertTrue(IsScriptComplete("println(\"{\");"));
   AssertTrue(IsScriptComplete("println(\"\\\"{\");"));
   AssertTrue(IsScriptComplete("println('{');"));
   AssertTrue(IsScriptComplete("println('\\');"));
   AssertTrue(IsScriptComplete("x = 1; // {"));
   AssertTrue(IsScriptComplete("x = 1; /* { */"));
   AssertFalse(IsScriptComplete("x = 1; // comment\nif (x) {"));

   // Unclosed block comments (can be nested) and text blocks
   AssertFalse(IsScriptComplete("/* comment"));
   AssertFalse(IsScriptComplete("/* outer /* inner */"));
   AssertTrue(IsScriptComplete("/* outer /* inner */ */"));
   AssertFalse(IsScriptComplete("x = \"\"\"text"));
   AssertTrue(IsScriptComplete("x = \"\"\"text {\n more\"\"\";"));

   // Unterminated string is complete input, error is reported by compiler
   AssertTrue(IsScriptComplete("println(\"abc);"));

   // Extra closing brackets are reported by compiler
   AssertTrue(IsScriptComplete("}"));

   EndTest();

   StartTest(_T("IsInputComplete"));

   AssertTrue(IsInputComplete(ShellMode::SHELL, "ls"));
   AssertTrue(IsInputComplete(ShellMode::SHELL, "cd weird{name"));
   AssertTrue(IsInputComplete(ShellMode::SHELL, "ai what does { mean?"));
   AssertTrue(IsInputComplete(ShellMode::SHELL, "console show {"));
   AssertFalse(IsInputComplete(ShellMode::SHELL, "nxsl if (true) {"));
   AssertTrue(IsInputComplete(ShellMode::SHELL, "nxsl if (true) {\n}"));
   AssertFalse(IsInputComplete(ShellMode::SHELL, "= max(1,"));
   AssertFalse(IsInputComplete(ShellMode::SHELL, "alias x {"));
   AssertTrue(IsInputComplete(ShellMode::SHELL, "alias x {\n   return 1;\n}"));
   AssertTrue(IsInputComplete(ShellMode::SHELL, "alias x = return 1;"));
   AssertFalse(IsInputComplete(ShellMode::NXSL, "for(a : b) {"));
   AssertTrue(IsInputComplete(ShellMode::NXSL, "/cd weird{name"));
   AssertFalse(IsInputComplete(ShellMode::AI, "/alias x {"));
   AssertTrue(IsInputComplete(ShellMode::AI, "what does { mean?"));
   AssertTrue(IsInputComplete(ShellMode::CONSOLE, "show {"));

   EndTest();
}

/**
 * Test alias command parsing
 */
static void TestAliasCommand()
{
   StartTest(_T("ParseAliasCommand"));

   std::string name, body;
   AssertTrue(ParseAliasCommand("", &name, &body) == AliasCommandType::LIST);
   AssertTrue(ParseAliasCommand("   ", &name, &body) == AliasCommandType::LIST);

   AssertTrue(ParseAliasCommand("Alarms", &name, &body) == AliasCommandType::SHOW);
   AssertEquals(name.c_str(), "alarms");

   AssertTrue(ParseAliasCommand("up = return $node.status;", &name, &body) == AliasCommandType::DEFINE);
   AssertEquals(name.c_str(), "up");
   AssertEquals(body.c_str(), "return $node.status;");

   AssertTrue(ParseAliasCommand("up=return 1;", &name, &body) == AliasCommandType::DEFINE);
   AssertEquals(name.c_str(), "up");
   AssertEquals(body.c_str(), "return 1;");

   AssertTrue(ParseAliasCommand("my-alias {\n   println(1);\n}", &name, &body) == AliasCommandType::DEFINE);
   AssertEquals(name.c_str(), "my-alias");
   AssertEquals(body.c_str(), "println(1);");

   AssertTrue(ParseAliasCommand("x{ if (a) { b(); } }", &name, &body) == AliasCommandType::DEFINE);
   AssertEquals(name.c_str(), "x");
   AssertEquals(body.c_str(), "if (a) { b(); }");

   AssertTrue(ParseAliasCommand("x =", &name, &body) == AliasCommandType::INVALID);
   AssertTrue(ParseAliasCommand("x { }", &name, &body) == AliasCommandType::INVALID);
   AssertTrue(ParseAliasCommand("x return 1;", &name, &body) == AliasCommandType::INVALID);
   AssertTrue(ParseAliasCommand("1x = return 1;", &name, &body) == AliasCommandType::INVALID);
   AssertTrue(ParseAliasCommand("a/b = return 1;", &name, &body) == AliasCommandType::INVALID);
   AssertTrue(ParseAliasCommand("= return 1;", &name, &body) == AliasCommandType::INVALID);

   AssertTrue(IsValidAliasName("a"));
   AssertTrue(IsValidAliasName("_a1-b"));
   AssertFalse(IsValidAliasName(""));
   AssertFalse(IsValidAliasName("-a"));
   AssertFalse(IsValidAliasName("a b"));

   EndTest();
}

/**
 * Check parsed object path
 */
static void AssertPath(const char *text, bool absolute, const char *elements)
{
   ObjectPath path;
   ParseObjectPath(text, &path);
   AssertTrue(path.absolute == absolute);

   std::string joined;
   for(size_t i = 0; i < path.elements.size(); i++)
   {
      if (i > 0)
         joined.append("|");
      joined.append(path.elements[i]);
   }
   AssertEquals(joined.c_str(), elements);
}

/**
 * Test object path handling
 */
static void TestObjectPath()
{
   StartTest(_T("ParseObjectPath"));

   AssertPath("", false, "");
   AssertPath("/", true, "");
   AssertPath("/Infrastructure/Core/router1", true, "Infrastructure|Core|router1");
   AssertPath("Core/router1", false, "Core|router1");
   AssertPath("../..", false, "..|..");
   AssertPath("./a//b/", false, ".|a|b");
   AssertPath("/Entire Network/10.0.0.0\\/24", true, "Entire Network|10.0.0.0/24");
   AssertPath("Gi0\\/1", false, "Gi0/1");
   AssertPath("a\\\\b", false, "a\\b");
   AssertPath("a\\ b", false, "a b");
   AssertPath("\"/Entire Network/Zone 1\"", true, "Entire Network|Zone 1");
   AssertPath("  /a/b  ", true, "a|b");

   EndTest();

   StartTest(_T("EscapePathElement"));

   AssertEquals(EscapePathElement("router1").c_str(), "router1");
   AssertEquals(EscapePathElement("Gi0/1").c_str(), "Gi0\\/1");
   AssertEquals(EscapePathElement("a\\b").c_str(), "a\\\\b");
   AssertEquals(EscapePathElement("Entire Network").c_str(), "Entire Network");

   // Escaped name is parsed back to the same single element
   AssertPath(EscapePathElement("10.0.0.0/24 \\ test").c_str(), false, "10.0.0.0/24 \\ test");

   EndTest();

   StartTest(_T("SplitPathForCompletion"));

   std::string directory, prefix;
   SplitPathForCompletion("", &directory, &prefix);
   AssertEquals(directory.c_str(), "");
   AssertEquals(prefix.c_str(), "");

   SplitPathForCompletion("rou", &directory, &prefix);
   AssertEquals(directory.c_str(), "");
   AssertEquals(prefix.c_str(), "rou");

   SplitPathForCompletion("/", &directory, &prefix);
   AssertEquals(directory.c_str(), "/");
   AssertEquals(prefix.c_str(), "");

   SplitPathForCompletion("/Infrastructure/Co", &directory, &prefix);
   AssertEquals(directory.c_str(), "/Infrastructure/");
   AssertEquals(prefix.c_str(), "Co");

   SplitPathForCompletion("../router1/Gi0\\/", &directory, &prefix);
   AssertEquals(directory.c_str(), "../router1/");
   AssertEquals(prefix.c_str(), "Gi0/");

   SplitPathForCompletion("a\\/b/c\\\\d", &directory, &prefix);
   AssertEquals(directory.c_str(), "a\\/b/");
   AssertEquals(prefix.c_str(), "c\\d");

   EndTest();
}

/**
 * Test removal of terminal control sequences
 */
static void TestStripTerminalSequences()
{
   StartTest(_T("StripTerminalSequences"));

   AssertEquals(StripTerminalSequences("plain text\n").c_str(), "plain text\n");
   AssertEquals(StripTerminalSequences("\x1b[1mbold\x1b[0m text").c_str(), "bold text");
   AssertEquals(StripTerminalSequences("\x1b[31;1mred\x1b[0m\n\x1b[2Kline").c_str(), "red\nline");
   AssertEquals(StripTerminalSequences("text\x1b[").c_str(), "text");
   AssertEquals(StripTerminalSequences("text\x1b").c_str(), "text");
   AssertEquals(StripTerminalSequences("").c_str(), "");

   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   TestClassifyInput();
   TestSplit();
   TestInputCompleteness();
   TestAliasCommand();
   TestObjectPath();
   TestStripTerminalSequences();

   InitiateProcessShutdown();
   return 0;
}
