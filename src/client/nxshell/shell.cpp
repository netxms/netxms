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
** File: shell.cpp
**
**/

#include "nxshell.h"

/**
 * Name of startup file
 */
#define STARTUP_FILE _T("nxshellrc")

/**
 * Command description
 */
struct CommandDescription
{
   const char *name;
   const char *arguments;
   const char *description;
};

/**
 * Builtin commands
 */
static const CommandDescription s_builtinCommands[] =
{
   { "ai", "[message]", "Send message to AI assistant or switch to AI assistant mode" },
   { "alias", "[name [= code | { code }]]", "List, show, or define aliases" },
   { "cd", "[path | #id | @name | -]", "Change current object" },
   { "con", "[command]", "Short form of \"console\"" },
   { "console", "[command]", "Execute server debug console command or switch to console mode" },
   { "exit", nullptr, "Leave current mode or exit the shell" },
   { "help", nullptr, "Show this help message" },
   { "ls", "[path]", "List child objects" },
   { "nxsl", "[code]", "Execute NXSL code or switch to NXSL mode" },
   { "pwd", nullptr, "Show path to current object" },
   { "status", nullptr, "Show current session information" },
   { "unalias", "<name>", "Remove alias" }
};

/**
 * Commands available only in AI assistant mode
 */
static const CommandDescription s_assistantCommands[] =
{
   { "clear", nullptr, "Clear chat history" },
   { "incident", "[id]", "Set incident context (clears context if used without argument)" }
};

/**
 * Default aliases
 */
static const struct
{
   const char *name;
   const char *body;
} s_defaultAliases[] =
{
   {
      "alarms",
      "if ($object == null)\n"
      "{\n"
      "   println(\"No current object\");\n"
      "   return;\n"
      "}\n"
      "severity = [\"Normal\", \"Warning\", \"Minor\", \"Major\", \"Critical\"];\n"
      "count = 0;\n"
      "for(a : $object.alarms)\n"
      "{\n"
      "   println(a.id .. \"  \" .. severity[a.severity] .. \"  \" .. a.message);\n"
      "   count++;\n"
      "}\n"
      "if (count == 0)\n"
      "   println(\"No active alarms\");"
   },
   {
      "dci",
      "try\n"
      "{\n"
      "   list = FindAllDCIs($object, null, $1);\n"
      "}\n"
      "catch\n"
      "{\n"
      "   println(\"Current object does not have data collection\");\n"
      "   return;\n"
      "}\n"
      "for(d : list)\n"
      "{\n"
      "   if (d.dataType != null)\n"
      "      println(d.id .. \"  \" .. d.description .. \" = \" .. d.currentValue);\n"
      "}"
   },
   {
      "info",
      "if ($object == null)\n"
      "{\n"
      "   println(\"No current object\");\n"
      "   return;\n"
      "}\n"
      "status = [\"Normal\", \"Warning\", \"Minor\", \"Major\", \"Critical\", \"Unknown\", \"Unmanaged\", \"Disabled\", \"Testing\"];\n"
      "println(\"Name:    \" .. $object.name);\n"
      "println(\"ID:      \" .. $object.id);\n"
      "println(\"Class:   \" .. classof($object));\n"
      "println(\"Status:  \" .. status[$object.status]);\n"
      "if ($object.alias != \"\")\n"
      "   println(\"Alias:   \" .. $object.alias);\n"
      "if ($object.ipAddr != null && $object.ipAddr != \"\" && $object.ipAddr != \"0.0.0.0\")\n"
      "   println(\"Address: \" .. $object.ipAddr);\n"
      "if ($object.comments != \"\")\n"
      "   println(\"Comments:\\n\" .. $object.comments);"
   },
   {
      "interfaces",
      "if ($node == null)\n"
      "{\n"
      "   println(\"Current object is not a node\");\n"
      "   return;\n"
      "}\n"
      "state = [\"Unknown\", \"Up\", \"Down\", \"Testing\", \"Dormant\", \"Not present\"];\n"
      "for(i : $node.interfaces)\n"
      "   println(i.ifIndex .. \"  \" .. state[i.operState] .. \"  \" .. i.name);"
   }
};

/**
 * Find default alias with given name
 */
static const char *FindDefaultAlias(const std::string& name)
{
   for(size_t i = 0; i < sizeof(s_defaultAliases) / sizeof(s_defaultAliases[0]); i++)
   {
      if (name == s_defaultAliases[i].name)
         return s_defaultAliases[i].body;
   }
   return nullptr;
}

/**
 * Check if given name is a builtin command
 */
static bool IsBuiltinCommand(const std::string& name)
{
   for(size_t i = 0; i < sizeof(s_builtinCommands) / sizeof(CommandDescription); i++)
   {
      if (name == s_builtinCommands[i].name)
         return true;
   }
   return (name == "quit");
}

/**
 * Get display name of given mode
 */
static const char *ModeName(ShellMode mode)
{
   switch(mode)
   {
      case ShellMode::AI:
         return "ai";
      case ShellMode::CONSOLE:
         return "console";
      case ShellMode::NXSL:
         return "nxsl";
      default:
         return "shell";
   }
}

/**
 * Shell constructor
 */
Shell::Shell(WebApiClient *client, ShellMode homeMode, bool terminalInput, bool interactive)
{
   m_client = client;
   m_terminalInput = terminalInput;
   m_interactive = interactive;
   m_exitRequested = false;
   m_mode = homeMode;
   m_homeMode = homeMode;
   m_chatId = 0;
   m_incidentId = 0;
   m_consoleSocket = nullptr;

   // Server name for prompt is server URL without scheme
   m_serverName = client->getServerUrl();
   size_t separator = m_serverName.find("://");
   if (separator != std::string::npos)
      m_serverName.erase(0, separator + 3);
}

/**
 * Shell destructor
 */
Shell::~Shell()
{
   closeConsoleSession();
   if (m_chatId != 0)
   {
      // User should not be asked to authenticate again just to delete the chat
      m_client->setAuthenticator(nullptr);
      m_client->deleteChat(m_chatId);
   }
}

/**
 * Get input prompt without decorations: server, current location, and mode if it is not shell mode
 */
std::string Shell::getPrompt() const
{
   std::string prompt(m_serverName);
   prompt.append(":").append(getLocationPath());
   if (m_mode != ShellMode::SHELL)
      prompt.append(" ").append(ModeName(m_mode));
   return prompt;
}

/**
 * Leave current mode. Shell exits if current mode is the mode it was started in.
 */
void Shell::leaveMode()
{
   if (m_mode == m_homeMode)
      m_exitRequested = true;
   else
      m_mode = m_homeMode;
}

/**
 * Execute complete input. Returns false if execution failed.
 */
bool Shell::execute(const std::string& input)
{
   InputLine line = ClassifyInput(m_mode, input);
   switch(line.target)
   {
      case InputTarget::COMMAND:
         return executeCommand(line.text);
      case InputTarget::AI:
         return sendChatMessage(line.text.c_str());
      case InputTarget::CONSOLE:
         return executeConsoleCommand(line.text);
      case InputTarget::NXSL:
         return executeScript(line.text, std::vector<std::string>(), nullptr);
      default:
         return true;
   }
}

/**
 * Execute mode command without arguments. Returns false if given command is not a mode command.
 */
bool Shell::executeModeCommand(const std::string& command, bool *success)
{
   ShellMode mode;
   if (command == "ai")
      mode = ShellMode::AI;
   else if ((command == "console") || (command == "con"))
      mode = ShellMode::CONSOLE;
   else if (command == "nxsl")
      mode = ShellMode::NXSL;
   else
      return false;

   // Console session is opened when mode is entered, so that lack of access is reported immediately
   *success = (mode != ShellMode::CONSOLE) || !m_interactive || (m_consoleSocket != nullptr) || openConsoleSession();
   if (*success)
      m_mode = mode;
   return true;
}

/**
 * Execute shell command (builtin or alias)
 */
bool Shell::executeCommand(const std::string& line)
{
   std::string command, arguments;
   SplitCommand(line, &command, &arguments);

   // Commands specific to current mode
   if (m_mode == ShellMode::AI)
   {
      if (command == "clear")
         return clearChat();
      if (command == "incident")
         return setIncidentContext(arguments);
   }

   bool success;
   if (executeModeCommand(command, &success))
      return success;

   if ((command == "exit") || (command == "quit"))
   {
      leaveMode();
      return true;
   }
   if (command == "help")
   {
      showHelp();
      return true;
   }
   if (command == "status")
   {
      showStatus();
      return true;
   }
   if (command == "cd")
      return changeLocation(arguments);
   if (command == "ls")
      return listObjects(arguments);
   if (command == "pwd")
   {
      std::string text = getLocationPath();
      text.append("\n");
      WriteToTerminalUtf8(text.c_str());
      return true;
   }
   if (command == "alias")
      return executeAliasCommand(arguments);
   if (command == "unalias")
      return removeAlias(arguments);

   auto alias = m_aliases.find(command);
   if (alias != m_aliases.end())
      return runAlias(command, alias->second, arguments);

   const char *defaultAlias = FindDefaultAlias(command);
   if (defaultAlias != nullptr)
      return runAlias(command, defaultAlias, arguments);

   PrintError("unknown command \"%s\" (use \"%shelp\" for list of available commands)", command.c_str(), (m_mode != ShellMode::SHELL) ? "/" : "");
   return false;
}

/**
 * Execute alias
 */
bool Shell::runAlias(const std::string& name, const std::string& body, const std::string& arguments)
{
   std::vector<std::string> parameters;
   if (!SplitArguments(arguments, &parameters))
   {
      PrintError("unterminated quote in arguments of \"%s\"", name.c_str());
      return false;
   }
   return executeScript(body, parameters, name.c_str());
}

/**
 * Print alias definition
 */
static void PrintAlias(const std::string& name, const std::string& body, bool isDefault)
{
   std::string text;
   AppendHighlightedText(&text, "36", name.c_str());
   if (isDefault)
      AppendHighlightedText(&text, "90", " (default)");
   if (body.find('\n') == std::string::npos)
   {
      text.append(" = ").append(body).append("\n");
   }
   else
   {
      text.append("\n{\n");
      size_t start = 0;
      while(start <= body.length())
      {
         size_t end = body.find('\n', start);
         if (end == std::string::npos)
            end = body.length();
         text.append("   ").append(body, start, end - start).append("\n");
         start = end + 1;
      }
      text.append("}\n");
   }
   WriteToTerminalUtf8(text.c_str());
}

/**
 * Execute "alias" command
 */
bool Shell::executeAliasCommand(const std::string& arguments)
{
   std::string name, body;
   switch(ParseAliasCommand(arguments, &name, &body))
   {
      case AliasCommandType::LIST:
      {
         // User aliases and default aliases that are not replaced, sorted by name
         std::map<std::string, bool> names;
         for(size_t i = 0; i < sizeof(s_defaultAliases) / sizeof(s_defaultAliases[0]); i++)
            names[s_defaultAliases[i].name] = true;
         for(auto it = m_aliases.begin(); it != m_aliases.end(); ++it)
            names[it->first] = false;

         std::string text;
         for(auto it = names.begin(); it != names.end(); ++it)
         {
            text.append("  ");
            AppendHighlightedText(&text, "36", it->first.c_str());
            if (it->second)
               AppendHighlightedText(&text, "90", " (default)");
            text.append("\n");
         }
         WriteToTerminalUtf8(text.c_str());
         return true;
      }
      case AliasCommandType::SHOW:
      {
         auto alias = m_aliases.find(name);
         if (alias != m_aliases.end())
         {
            PrintAlias(name, alias->second, false);
            return true;
         }
         const char *defaultAlias = FindDefaultAlias(name);
         if (defaultAlias != nullptr)
         {
            PrintAlias(name, defaultAlias, true);
            return true;
         }
         PrintError("alias \"%s\" is not defined", name.c_str());
         return false;
      }
      case AliasCommandType::DEFINE:
         if (IsBuiltinCommand(name))
         {
            PrintError("\"%s\" is a builtin command and cannot be redefined", name.c_str());
            return false;
         }
         m_aliases[name] = body;
         return true;
      default:
         PrintError("usage: alias [name [= code | { code }]]");
         return false;
   }
}

/**
 * Remove user defined alias
 */
bool Shell::removeAlias(const std::string& name)
{
   std::string key(name);
   ToLowerCase(&key);
   if (key.empty())
   {
      PrintError("usage: unalias <name>");
      return false;
   }

   if (m_aliases.erase(key) > 0)
      return true;

   if (FindDefaultAlias(key) != nullptr)
      PrintError("\"%s\" is a default alias and cannot be removed", key.c_str());
   else
      PrintError("alias \"%s\" is not defined", key.c_str());
   return false;
}

/**
 * Append command descriptions to help text
 */
static void AppendCommandHelp(std::string *text, const CommandDescription *commands, size_t count, const char *prefix)
{
   for(size_t i = 0; i < count; i++)
   {
      std::string command(prefix);
      command.append(commands[i].name);
      if (commands[i].arguments != nullptr)
         command.append(" ").append(commands[i].arguments);

      text->append("  ");
      AppendHighlightedText(text, "36", command.c_str());
      text->append((command.length() < 36) ? 36 - command.length() : 1, ' ');
      text->append(commands[i].description).append("\n");
   }
}

/**
 * Show help
 */
void Shell::showHelp()
{
   const char *prefix = (m_mode != ShellMode::SHELL) ? "/" : "";

   std::string text("\n");
   AppendHighlightedText(&text, "1", "Available commands:");
   text.append("\n\n");
   AppendCommandHelp(&text, s_builtinCommands, sizeof(s_builtinCommands) / sizeof(CommandDescription), prefix);
   if (m_mode == ShellMode::AI)
      AppendCommandHelp(&text, s_assistantCommands, sizeof(s_assistantCommands) / sizeof(CommandDescription), prefix);
   text.append("  ");
   AppendHighlightedText(&text, "36", (std::string(prefix) + "= <expression>").c_str());
   text.append(std::string(36 - strlen(prefix) - 14, ' ')).append("Evaluate NXSL expression\n");

   text.append("\n");
   if (m_mode != ShellMode::SHELL)
   {
      text.append("Input is sent to ").append(ModeName(m_mode)).append(" unless it starts with \"/\" followed by a command.\n");
   }
   else
   {
      text.append("Commands ai, console, and nxsl used without arguments switch to corresponding mode,\n"
                  "where every line is sent to that target. Aliases are commands defined as NXSL code\n"
                  "executed for current object (use \"alias\" command to list them).\n");
   }

   text.append("\n");
   AppendHighlightedText(&text, "1", "Keyboard shortcuts:");
   text.append("\n\n  ");
   AppendHighlightedText(&text, "36", "Ctrl+C");
   text.append("    Cancel current operation or input\n  ");
   AppendHighlightedText(&text, "36", "Ctrl+D");
   text.append("    Leave current mode or exit the shell\n\n");

   WriteToTerminalUtf8(text.c_str());
}

/**
 * Show current session information
 */
void Shell::showStatus()
{
   std::string text("\n");
   AppendHighlightedText(&text, "1", "Session status:");
   text.append("\n\n");

   char line[1024];
   snprintf(line, sizeof(line), "  Server:   %s\n  Mode:     %s\n  Location: %s", m_client->getServerUrl(), ModeName(m_mode), getLocationPath().c_str());
   text.append(line);
   if (!m_location.empty())
   {
      snprintf(line, sizeof(line), " (%s [%u])", m_location.back().className.c_str(), m_location.back().id);
      text.append(line);
   }
   text.append("\n");

   if (m_chatId != 0)
      snprintf(line, sizeof(line), "  AI chat:  %u\n", m_chatId);
   else
      strcpy(line, "  AI chat:  not started\n");
   text.append(line);

   if (m_incidentId != 0)
   {
      snprintf(line, sizeof(line), "  Incident: %u\n", m_incidentId);
      text.append(line);
   }

   text.append("  Console:  ").append((m_consoleSocket != nullptr) ? "session open" : "no session").append("\n\n");
   WriteToTerminalUtf8(text.c_str());
}

/**
 * Create context document for message to AI assistant. Incident set as conversation context takes
 * precedence over current object. Returns nullptr if there is no context.
 */
json_t *Shell::createChatContext() const
{
   uint32_t objectId = getCurrentObjectId();
   if ((objectId == 0) && (m_incidentId == 0))
      return nullptr;

   json_t *context = json_object();
   if (m_incidentId != 0)
      json_object_set_new(context, "incidentId", json_integer(m_incidentId));
   else
      json_object_set_new(context, "objectId", json_integer(objectId));
   return context;
}

/**
 * Send message to AI assistant, answer questions asked during processing, and render response
 */
bool Shell::sendChatMessage(const char *message)
{
   if ((m_chatId == 0) && !m_client->createChat(m_incidentId, (m_incidentId == 0) ? getCurrentObjectId() : 0, &m_chatId))
   {
      PrintError("%s", m_client->getErrorText());
      return false;
   }

   json_t *context = createChatContext();
   ChatResponse response;
   bool success = m_client->sendMessage(m_chatId, message, context, &response, m_progressCallback);
   json_decref(context);

   while(success && (response.question.id != 0))
   {
      ProgressIndicatorStop();

      bool positive = false;
      int selectedOption = -1;
      if (m_terminalInput)
      {
         // Question is declined if user cancels input
         PromptForAnswer(response.question, &positive, &selectedOption);
      }
      else
      {
         PrintWarning("assistant asked a question that requires interactive session, declining");
         PrintStatus("%s", response.question.text.c_str());
      }

      success = m_client->answerQuestion(m_chatId, response.question.id, positive, selectedOption) &&
                m_client->waitForResponse(m_chatId, &response, m_progressCallback);
   }

   ProgressIndicatorStop();

   if (!success)
   {
      PrintError("%s", m_client->getErrorText());
      return false;
   }

   if (!response.text.empty())
      RenderResponse(response.text.c_str());
   return true;
}

/**
 * Set or clear incident used as AI assistant conversation context
 */
bool Shell::setIncidentContext(const std::string& arguments)
{
   if (arguments.empty())
   {
      if (m_incidentId == 0)
      {
         PrintError("usage: /incident <id>");
         return false;
      }
      m_incidentId = 0;
      PrintStatus("Incident context cleared");
      return true;
   }

   char *eptr;
   uint32_t incidentId = strtoul(arguments.c_str(), &eptr, 0);
   if ((*eptr != 0) || (incidentId == 0))
   {
      PrintError("invalid incident ID \"%s\"", arguments.c_str());
      return false;
   }

   m_incidentId = incidentId;
   PrintSuccess("Conversation context set to incident [%u]", incidentId);
   return true;
}

/**
 * Clear AI assistant chat history
 */
bool Shell::clearChat()
{
   if ((m_chatId != 0) && !m_client->clearChat(m_chatId))
   {
      PrintError("%s", m_client->getErrorText());
      return false;
   }
   PrintSuccess("Chat history cleared");
   return true;
}

/**
 * Read line of any length from stream. Returns false on end of file.
 */
static bool ReadLine(FILE *stream, std::string *line)
{
   line->clear();
   char buffer[4096];
   while(fgets(buffer, sizeof(buffer), stream) != nullptr)
   {
      line->append(buffer);
      if (!line->empty() && (line->back() == '\n'))
      {
         line->pop_back();
         if (!line->empty() && (line->back() == '\r'))
            line->pop_back();
         return true;
      }
   }
   return !line->empty();
}

/**
 * Execute commands from stream. Input that spans multiple lines is accumulated until it is complete.
 * If stopOnError is set, execution stops at first failed command, otherwise failures are reported
 * as warnings with location in the stream. Returns false if any command failed.
 */
bool Shell::executeStream(FILE *stream, const char *name, bool stopOnError)
{
   bool success = true;
   std::string input, line;
   int lineNumber = 0, startLine = 0;
   while(!m_exitRequested && ReadLine(stream, &line))
   {
      lineNumber++;
      if (input.empty())
      {
         TrimString(&line);
         if (line.empty())
            continue;
         input = line;
         startLine = lineNumber;
      }
      else
      {
         input.append("\n").append(line);
      }

      if (!isInputComplete(input))
         continue;

      bool result = execute(input);
      input.clear();
      if (!result)
      {
         success = false;
         if (stopOnError)
            return false;
         PrintWarning("command at line %d of %s failed", startLine, name);
      }
   }

   if (!input.empty())
   {
      PrintError("unexpected end of input in command at line %d of %s", startLine, name);
      success = false;
   }
   return success;
}

/**
 * Execute startup file if it exists. Failures do not stop execution and shell always returns
 * to the mode it was started in.
 */
void Shell::executeStartupFile()
{
   TCHAR path[MAX_PATH];
   if (!GetConfigFilePath(STARTUP_FILE, path, MAX_PATH))
      return;

   FILE *stream = _tfopen(path, _T("r"));
   if (stream == nullptr)
      return;

   // Startup file is always processed as shell mode input
   m_mode = ShellMode::SHELL;
   executeStream(stream, "startup file", false);
   fclose(stream);

   m_mode = m_homeMode;
   m_exitRequested = false;
}

/**
 * Get length of common prefix of two strings, ignoring case of ASCII characters
 */
static size_t CommonPrefixLength(const std::string& s1, const std::string& s2)
{
   size_t length = 0;
   while((length < s1.length()) && (length < s2.length()) && (tolower(static_cast<unsigned char>(s1[length])) == tolower(static_cast<unsigned char>(s2[length]))))
      length++;
   return length;
}

/**
 * Complete input at cursor position. Input is the text from start of line to cursor. On return
 * completion contains text to be inserted at cursor position (can be empty), and candidates
 * contains all matching names if there is more than one.
 */
void Shell::complete(const std::string& input, std::string *completion, std::vector<std::string> *candidates)
{
   completion->clear();
   candidates->clear();

   // Only shell commands can be completed
   std::string line(input);
   size_t start = line.find_first_not_of(" \t");
   line.erase(0, (start != std::string::npos) ? start : line.length());
   if (m_mode != ShellMode::SHELL)
   {
      if (line.empty() || (line[0] != '/'))
         return;
      line.erase(0, 1);
   }

   std::vector<std::string> names;   // Unescaped names matching entered prefix
   std::string prefix;
   bool pathCompletion = false;

   size_t separator = line.find_first_of(" \t");
   if (separator == std::string::npos)
   {
      // Command name
      prefix = line;
      for(size_t i = 0; i < sizeof(s_builtinCommands) / sizeof(CommandDescription); i++)
         names.push_back(s_builtinCommands[i].name);
      if (m_mode == ShellMode::AI)
      {
         for(size_t i = 0; i < sizeof(s_assistantCommands) / sizeof(CommandDescription); i++)
            names.push_back(s_assistantCommands[i].name);
      }
      for(auto it = m_aliases.begin(); it != m_aliases.end(); ++it)
         names.push_back(it->first);
      for(size_t i = 0; i < sizeof(s_defaultAliases) / sizeof(s_defaultAliases[0]); i++)
      {
         if (m_aliases.find(s_defaultAliases[i].name) == m_aliases.end())
            names.push_back(s_defaultAliases[i].name);
      }
   }
   else
   {
      // Object path in arguments of "cd" and "ls"
      std::string command = line.substr(0, separator);
      ToLowerCase(&command);
      if ((command != "cd") && (command != "ls"))
         return;

      size_t argumentStart = line.find_first_not_of(" \t", separator);
      std::string argument = (argumentStart != std::string::npos) ? line.substr(argumentStart) : std::string();
      if (!argument.empty() && ((argument[0] == '#') || (argument[0] == '@') || (argument[0] == '"')))
         return;

      std::string directory;
      SplitPathForCompletion(argument, &directory, &prefix);

      std::vector<ObjectInfo> location;
      if (!resolvePath(directory, &location, false))
         return;

      const std::vector<ObjectInfo> *objects;
      if (!getChildObjects(location.empty() ? 0 : location.back().id, &objects))
         return;

      for(size_t i = 0; i < objects->size(); i++)
         names.push_back(objects->at(i).name);
      pathCompletion = true;
   }

   std::string common;
   for(size_t i = 0; i < names.size(); i++)
   {
      if ((names[i].length() < prefix.length()) || (CommonPrefixLength(names[i], prefix) != prefix.length()))
         continue;
      if (candidates->empty())
         common = names[i];
      else
         common.erase(CommonPrefixLength(common, names[i]));
      candidates->push_back(names[i]);
   }

   if (candidates->empty())
      return;

   // Multibyte character should not be split
   while((common.length() > prefix.length()) && (common.length() < candidates->front().length()) &&
         ((static_cast<unsigned char>(candidates->front()[common.length()]) & 0xC0) == 0x80))
      common.pop_back();

   if (common.length() > prefix.length())
   {
      std::string remainder = common.substr(prefix.length());
      *completion = pathCompletion ? EscapePathElement(remainder) : remainder;
   }

   if (candidates->size() == 1)
   {
      completion->append(pathCompletion ? "/" : " ");
      candidates->clear();
   }
}
