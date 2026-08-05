/*
** NetXMS - Network Management System
** Command line AI assistant client
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
** File: repl.cpp
**
**/

#include "nxai.h"
#include <signal.h>

#if HAVE_LIBEDIT
#include <histedit.h>
#endif

/**
 * Number of commands kept in history file
 */
#define HISTORY_SIZE    200

/**
 * Interval between progress indicator updates
 */
#define SPINNER_INTERVAL   150

/**
 * Progress indicator frames
 */
static const char *s_spinnerFrames[] = { "-", "\\", "|", "/" };

/**
 * Progress indicator displayed while assistant is processing request
 */
class ProgressIndicator
{
private:
   Mutex m_mutex;
   Condition m_stopCondition;
   THREAD m_thread;
   std::string m_message;

   void run();

public:
   ProgressIndicator() : m_mutex(MutexType::FAST), m_stopCondition(true)
   {
      m_thread = INVALID_THREAD_HANDLE;
   }
   ~ProgressIndicator()
   {
      stop();
   }

   void update(const char *currentFunction);
   void stop();
};

/**
 * Progress indicator thread
 */
void ProgressIndicator::run()
{
   int frame = 0;
   while(!m_stopCondition.wait(SPINNER_INTERVAL))
   {
      m_mutex.lock();
      std::string message = m_message;
      m_mutex.unlock();

      std::string text("\r\x1b[2K\x1b[36m");
      text.append(s_spinnerFrames[frame]);
      text.append("\x1b[0m ").append(message);
      WriteToTerminalUtf8(text.c_str());
      fflush(stdout);

      frame = (frame + 1) % static_cast<int>(sizeof(s_spinnerFrames) / sizeof(const char*));
   }

   WriteToTerminalUtf8("\r\x1b[2K");
   fflush(stdout);
}

/**
 * Update text displayed by progress indicator. Indicator is started if it is not running yet.
 */
void ProgressIndicator::update(const char *currentFunction)
{
   LockGuard lockGuard(m_mutex);

   if (currentFunction != nullptr)
   {
      m_message = "Executing ";
      m_message.append(currentFunction).append("...");
   }
   else
   {
      m_message = "Thinking...";
   }

   if (m_thread == INVALID_THREAD_HANDLE)
      m_thread = ThreadCreateEx([this] () { run(); });
}

/**
 * Stop progress indicator and erase it from screen. Can be called when indicator is not running.
 */
void ProgressIndicator::stop()
{
   m_mutex.lock();
   THREAD thread = m_thread;
   m_thread = INVALID_THREAD_HANDLE;
   m_mutex.unlock();

   if (thread == INVALID_THREAD_HANDLE)
      return;

   m_stopCondition.set();
   ThreadJoin(thread);
   m_stopCondition.reset();
}

/**
 * Progress indicator instance
 */
static ProgressIndicator s_progressIndicator;

/**
 * Show progress indicator with name of function currently executed by assistant (can be nullptr)
 */
void ProgressIndicatorUpdate(const char *currentFunction)
{
   s_progressIndicator.update(currentFunction);
}

/**
 * Hide progress indicator
 */
void ProgressIndicatorStop()
{
   s_progressIndicator.stop();
}

/**
 * Number of interrupt signals received since last reset
 */
static VolatileCounter s_interrupted = 0;

/**
 * Client to be notified when interrupt signal is received
 */
static WebApiClient *s_activeClient = nullptr;

/**
 * Interrupt signal handler
 */
static void OnInterrupt(int signalCode)
{
   InterlockedIncrement(&s_interrupted);
   if (s_activeClient != nullptr)
      s_activeClient->cancel();
#ifdef _WIN32
   signal(SIGINT, OnInterrupt);
#endif
}

/**
 * Install interrupt signal handler
 */
static void SetInterruptHandler(WebApiClient *client)
{
   s_activeClient = client;
#ifdef _WIN32
   signal(SIGINT, OnInterrupt);
#else
   struct sigaction action;
   memset(&action, 0, sizeof(action));
   action.sa_handler = OnInterrupt;
   sigemptyset(&action.sa_mask);
   action.sa_flags = 0;   // Blocking calls should be interrupted instead of restarted
   sigaction(SIGINT, &action, nullptr);
#endif
}

/**
 * Slash command definition
 */
struct SlashCommand
{
   const char *name;
   const char *arguments;
   const char *description;
};

/**
 * Supported slash commands
 */
static const SlashCommand s_slashCommands[] =
{
   { "clear", nullptr, "Clear chat history" },
   { "exit", nullptr, "Exit the chat" },
   { "help", nullptr, "Show this help message" },
   { "incident", "<id>", "Set incident context (clears context if used without argument)" },
   { "object", "<name>", "Set object context (clears context if used without argument)" },
   { "quit", nullptr, "Exit the chat" },
   { "status", nullptr, "Show current session information" }
};

#define SLASH_COMMAND_COUNT   (sizeof(s_slashCommands) / sizeof(SlashCommand))

/**
 * Convert ASCII characters in given string to lower case
 */
static void ToLowerCase(std::string *text)
{
   for(size_t i = 0; i < text->length(); i++)
   {
      char ch = (*text)[i];
      if ((ch >= 'A') && (ch <= 'Z'))
         (*text)[i] = ch + ('a' - 'A');
   }
}

/**
 * Append text with given SGR attributes. Attributes are ignored in plain output mode.
 */
static void AppendHighlightedText(std::string *output, const char *attributes, const char *text)
{
   if (!g_plainOutput)
      output->append("\x1b[").append(attributes).append("m");
   output->append(text);
   if (!g_plainOutput)
      output->append("\x1b[0m");
}

/**
 * Show list of available commands
 */
static void ShowCommandHelp()
{
   std::string text("\n");
   AppendHighlightedText(&text, "1", "Available commands:");
   text.append("\n\n");

   for(size_t i = 0; i < SLASH_COMMAND_COUNT; i++)
   {
      char command[64];
      snprintf(command, sizeof(command), "/%s%s%s", s_slashCommands[i].name,
            (s_slashCommands[i].arguments != nullptr) ? " " : "",
            (s_slashCommands[i].arguments != nullptr) ? s_slashCommands[i].arguments : "");

      char padding[32];
      size_t length = strlen(command);
      size_t paddingLength = (length < 20) ? 20 - length : 1;
      memset(padding, ' ', paddingLength);
      padding[paddingLength] = 0;

      text.append("  ");
      AppendHighlightedText(&text, "36", command);
      text.append(padding).append(s_slashCommands[i].description).append("\n");
   }

   text.append("\n");
   AppendHighlightedText(&text, "1", "Keyboard shortcuts:");
   text.append("\n\n");
   text.append("  ");
   AppendHighlightedText(&text, "36", "Ctrl+C");
   text.append("              Cancel current request or input\n");
   text.append("  ");
   AppendHighlightedText(&text, "36", "Ctrl+D");
   text.append("              Exit the chat\n\n");

   WriteToTerminalUtf8(text.c_str());
}

/**
 * Show current session information
 */
static void ShowSessionStatus(const ChatSession *session)
{
   std::string text("\n");
   AppendHighlightedText(&text, "1", "Session status:");
   text.append("\n\n");

   char line[1024];
   snprintf(line, sizeof(line), "  Server:  %s\n  Chat ID: %u\n", session->server.c_str(), session->chatId);
   text.append(line);

   if (session->objectId != 0)
      snprintf(line, sizeof(line), "  Context: object [%u]\n", session->objectId);
   else if (session->incidentId != 0)
      snprintf(line, sizeof(line), "  Context: incident [%u]\n", session->incidentId);
   else
      strcpy(line, "  Context: none\n");
   text.append(line).append("\n");

   WriteToTerminalUtf8(text.c_str());
}

/**
 * Set object given by name as conversation context
 */
static void SetObjectContext(ChatSession *session, const char *name)
{
   ObjectInfo object;
   if (!session->client->findObject(name, &object))
   {
      PrintError("%s", session->client->getErrorText());
      return;
   }

   if (object.id == 0)
   {
      PrintError("object \"%s\" not found", name);
      return;
   }

   session->objectId = object.id;
   session->incidentId = 0;
   PrintSuccess("Conversation context set to %s [%u]", object.name.c_str(), object.id);
}

/**
 * Set incident given by ID as conversation context
 */
static void SetIncidentContext(ChatSession *session, const char *id)
{
   char *eptr;
   uint32_t incidentId = strtoul(id, &eptr, 0);
   if ((*eptr != 0) || (incidentId == 0))
   {
      PrintError("invalid incident ID \"%s\"", id);
      return;
   }

   session->incidentId = incidentId;
   session->objectId = 0;
   PrintSuccess("Conversation context set to incident [%u]", incidentId);
}

/**
 * Execute slash command. Returns false if session should be terminated.
 */
static bool ExecuteSlashCommand(ChatSession *session, const std::string& input)
{
   std::string command, arguments;
   size_t separator = input.find_first_of(" \t");
   if (separator != std::string::npos)
   {
      command = input.substr(1, separator - 1);
      arguments = input.substr(separator + 1);
      TrimString(&arguments);
   }
   else
   {
      command = input.substr(1);
   }
   ToLowerCase(&command);

   if ((command == "quit") || (command == "exit") || (command == "q"))
      return false;

   if (command == "help")
   {
      ShowCommandHelp();
   }
   else if (command == "clear")
   {
      if (session->client->clearChat(session->chatId))
         PrintSuccess("Chat history cleared");
      else
         PrintError("%s", session->client->getErrorText());
   }
   else if (command == "object")
   {
      if (!arguments.empty())
         SetObjectContext(session, arguments.c_str());
      else if (session->objectId != 0)
      {
         session->objectId = 0;
         PrintStatus("Object context cleared");
      }
      else
         PrintError("usage: /object <name>");
   }
   else if (command == "incident")
   {
      if (!arguments.empty())
         SetIncidentContext(session, arguments.c_str());
      else if (session->incidentId != 0)
      {
         session->incidentId = 0;
         PrintStatus("Incident context cleared");
      }
      else
         PrintError("usage: /incident <id>");
   }
   else if (command == "status")
   {
      ShowSessionStatus(session);
   }
   else
   {
      PrintError("unknown command /%s (use /help for list of available commands)", command.c_str());
   }

   return true;
}

/**
 * Get labels for positive and negative answers on confirmation question
 */
static void GetConfirmationLabels(ConfirmationType type, const char **positive, const char **negative)
{
   switch(type)
   {
      case ConfirmationType::YES_NO:
         *positive = "yes";
         *negative = "no";
         break;
      case ConfirmationType::CONFIRM_CANCEL:
         *positive = "confirm";
         *negative = "cancel";
         break;
      default:
         *positive = "approve";
         *negative = "reject";
         break;
   }
}

/**
 * Display question asked by assistant
 */
static void RenderQuestion(const Question& question)
{
   std::string text("\n");
   AppendHighlightedText(&text, "34;1", "AI assistant is asking:");
   text.append("\n").append(question.text).append("\n");

   if (!question.context.empty())
   {
      text.append("\n");
      AppendHighlightedText(&text, "90", "Context: ");
      AppendHighlightedText(&text, "90", question.context.c_str());
      text.append("\n");
   }

   if (question.multipleChoice && !question.options.empty())
   {
      text.append("\nOptions:\n");
      for(size_t i = 0; i < question.options.size(); i++)
      {
         char prefix[32];
         snprintf(prefix, sizeof(prefix), "  %d. ", static_cast<int>(i) + 1);
         text.append(prefix).append(question.options[i]).append("\n");
      }
   }
   text.append("\n");

   WriteToTerminalUtf8(text.c_str());
}

/**
 * Ask user to select one of the options offered by assistant. Returns false if user cancelled input.
 */
static bool PromptForOption(const Question& question, int *selectedOption)
{
   int optionCount = static_cast<int>(question.options.size());

   char prompt[64];
   snprintf(prompt, sizeof(prompt), "Enter choice (1-%d): ", optionCount);

   while(true)
   {
      std::string answer;
      if (!ReadInputLine(prompt, &answer))
         return false;

      TrimString(&answer);
      if (answer.empty())
         return false;   // Empty input cancels selection

      char *eptr;
      long choice = strtol(answer.c_str(), &eptr, 10);
      if ((*eptr == 0) && (choice >= 1) && (choice <= optionCount))
      {
         *selectedOption = static_cast<int>(choice) - 1;
         return true;
      }

      PrintError("enter number between 1 and %d", optionCount);
   }
}

/**
 * Ask user to confirm or reject action proposed by assistant. Returns false if user cancelled input.
 */
static bool PromptForConfirmation(const Question& question, bool *positive)
{
   const char *positiveLabel, *negativeLabel;
   GetConfirmationLabels(question.confirmationType, &positiveLabel, &negativeLabel);

   char prompt[64];
   snprintf(prompt, sizeof(prompt), "[%s/%s]: ", positiveLabel, negativeLabel);

   while(true)
   {
      std::string answer;
      if (!ReadInputLine(prompt, &answer))
         return false;

      TrimString(&answer);
      ToLowerCase(&answer);

      if ((answer == positiveLabel) || ((answer.length() == 1) && (answer[0] == positiveLabel[0])))
      {
         *positive = true;
         return true;
      }
      if ((answer == negativeLabel) || ((answer.length() == 1) && (answer[0] == negativeLabel[0])))
      {
         *positive = false;
         return true;
      }

      PrintError("enter \"%s\" or \"%s\"", positiveLabel, negativeLabel);
   }
}

/**
 * Display question asked by assistant and read answer from user. Question is considered declined
 * if user cancels input.
 */
bool PromptForAnswer(const Question& question, bool *positive, int *selectedOption)
{
   *positive = false;
   *selectedOption = -1;

   RenderQuestion(question);

   bool answered;
   if (question.multipleChoice && !question.options.empty())
   {
      answered = PromptForOption(question, selectedOption);
      if (answered)
      {
         *positive = true;
         PrintStatus("Selected: %s", question.options[*selectedOption].c_str());
      }
   }
   else
   {
      answered = PromptForConfirmation(question, positive);
   }

   if (!answered)
   {
      WriteToTerminalUtf8("\n");
      PrintStatus("Question declined");
      return false;
   }

   if (!question.multipleChoice)
   {
      const char *positiveLabel, *negativeLabel;
      GetConfirmationLabels(question.confirmationType, &positiveLabel, &negativeLabel);
      PrintStatus("Responded: %s", *positive ? positiveLabel : negativeLabel);
   }
   return true;
}

#if HAVE_LIBEDIT

/**
 * Command line editor
 */
static EditLine *s_editLine = nullptr;

/**
 * Command history
 */
static History *s_commandHistory = nullptr;

/**
 * Path to command history file
 */
static char s_historyFile[MAX_PATH] = "";

/**
 * Get input prompt for command line editor
 */
static char *EditLinePrompt(EditLine *el)
{
#ifdef EL_PROMPT_ESC
   static char prompt[] = "\1\x1b[36;1m\1You>\1\x1b[0m\1 ";
#else
   static char prompt[] = "You> ";
#endif
   static char plainPrompt[] = "You> ";
   return g_plainOutput ? plainPrompt : prompt;
}

/**
 * Complete slash command at cursor position
 */
static unsigned char CompleteSlashCommand(EditLine *el, int ch)
{
   const LineInfo *lineInfo = el_line(el);
   size_t length = lineInfo->cursor - lineInfo->buffer;
   if ((length == 0) || (lineInfo->buffer[0] != '/'))
      return CC_NORM;

   std::string prefix(lineInfo->buffer + 1, length - 1);
   if (prefix.find_first_of(" \t") != std::string::npos)
      return CC_NORM;   // Command is already entered, arguments cannot be completed

   const SlashCommand *match = nullptr;
   size_t matchCount = 0;
   for(size_t i = 0; i < SLASH_COMMAND_COUNT; i++)
   {
      if (!strncmp(s_slashCommands[i].name, prefix.c_str(), prefix.length()))
      {
         match = &s_slashCommands[i];
         matchCount++;
      }
   }

   if (matchCount == 0)
      return CC_ERROR;

   if (matchCount == 1)
   {
      std::string completion(match->name + prefix.length());
      if (match->arguments != nullptr)
         completion.append(" ");
      return (el_insertstr(el, completion.c_str()) == -1) ? CC_ERROR : CC_REFRESH;
   }

   // Show all matching commands
   std::string text("\n");
   for(size_t i = 0; i < SLASH_COMMAND_COUNT; i++)
   {
      if (!strncmp(s_slashCommands[i].name, prefix.c_str(), prefix.length()))
         text.append("  /").append(s_slashCommands[i].name);
   }
   text.append("\n");
   WriteToTerminalUtf8(text.c_str());
   fflush(stdout);
   return CC_REDISPLAY;
}

#endif   /* HAVE_LIBEDIT */

/**
 * Initialize command line editor
 */
static void InitializeLineEditor()
{
#if HAVE_LIBEDIT
   s_commandHistory = history_init();
   if (s_commandHistory != nullptr)
   {
      HistEvent historyEvent;
      history(s_commandHistory, &historyEvent, H_SETSIZE, HISTORY_SIZE);

      TCHAR path[MAX_PATH];
      if (GetConfigFilePath(_T("history"), path, MAX_PATH))
      {
         size_t bytes = tchar_to_utf8(path, -1, s_historyFile, MAX_PATH - 1);
         s_historyFile[bytes] = 0;
         history(s_commandHistory, &historyEvent, H_LOAD, s_historyFile);
      }
   }

   s_editLine = el_init("nxai", stdin, stdout, stderr);
#ifdef EL_PROMPT_ESC
   el_set(s_editLine, EL_PROMPT_ESC, EditLinePrompt, '\1');
#else
   el_set(s_editLine, EL_PROMPT, EditLinePrompt);
#endif
   el_set(s_editLine, EL_EDITOR, "emacs");
   el_set(s_editLine, EL_SIGNAL, 1);
   if (s_commandHistory != nullptr)
      el_set(s_editLine, EL_HIST, history, s_commandHistory);
   el_source(s_editLine, nullptr);

   // Completion is bound after reading user's configuration file, so that it cannot be overridden
   el_set(s_editLine, EL_ADDFN, "nxai-complete", "Complete slash command", CompleteSlashCommand);
   el_set(s_editLine, EL_BIND, "^I", "nxai-complete", nullptr);
#endif
}

/**
 * Save command history and destroy command line editor
 */
static void ShutdownLineEditor()
{
#if HAVE_LIBEDIT
   if (s_editLine != nullptr)
   {
      el_end(s_editLine);
      s_editLine = nullptr;
   }

   if (s_commandHistory != nullptr)
   {
      if (s_historyFile[0] != 0)
      {
         HistEvent historyEvent;
         history(s_commandHistory, &historyEvent, H_SAVE, s_historyFile);
      }
      history_end(s_commandHistory);
      s_commandHistory = nullptr;
   }
#endif
}

/**
 * Add command to history
 */
static void AddToHistory(const char *command)
{
#if HAVE_LIBEDIT
   if (s_commandHistory != nullptr)
   {
      HistEvent historyEvent;
      history(s_commandHistory, &historyEvent, H_ENTER, command);
   }
#endif
}

/**
 * Read command line from user. Returns false on end of input or if input was interrupted.
 */
static bool ReadCommandLine(std::string *line)
{
#if HAVE_LIBEDIT
   int count;
   const char *text = el_gets(s_editLine, &count);
   if ((text == nullptr) || (count <= 0))
      return false;
   line->assign(text, count);
   return true;
#else
   return ReadInputLine(g_plainOutput ? "You> " : "\x1b[36;1mYou>\x1b[0m ", line);
#endif
}

/**
 * Show welcome message
 */
static void ShowWelcome(const ChatSession *session)
{
   std::string text("\n");
   AppendHighlightedText(&text, "34;1", "NetXMS AI Assistant");
   text.append("\n");
   AppendHighlightedText(&text, "90", "Connected to ");
   AppendHighlightedText(&text, "90", session->server.c_str());
   text.append("\n");
   AppendHighlightedText(&text, "90", "Type your questions or commands. Use /help for list of available commands.");
   text.append("\n\n");
   WriteToTerminalUtf8(text.c_str());
}

/**
 * Run interactive chat session
 */
int RunChatSession(ChatSession *session)
{
   SetInterruptHandler(session->client);
   InitializeLineEditor();
   ShowWelcome(session);

   while(true)
   {
      InterlockedAnd(&s_interrupted, 0);
      session->client->resetCancellation();

      std::string input;
      if (!ReadCommandLine(&input))
      {
         WriteToTerminalUtf8("\n");
         if (s_interrupted > 0)
            continue;   // Input was interrupted by Ctrl+C, start new line
         break;   // End of input (Ctrl+D)
      }

      TrimString(&input);
      if (input.empty())
         continue;

      AddToHistory(input.c_str());

      if (input[0] == '/')
      {
         if (!ExecuteSlashCommand(session, input))
            break;
         continue;
      }

      session->sendMessage(input.c_str());
   }

   ProgressIndicatorStop();
   ShutdownLineEditor();
   s_activeClient = nullptr;
   PrintStatus("Goodbye!");
   return 0;
}
