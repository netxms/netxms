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
** File: repl.cpp
**
**/

#include "nxshell.h"
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

/**
 * Ask user to confirm an action. Returns false if user declined or cancelled input.
 */
bool AskConfirmation(const char *question)
{
   std::string prompt(question);
   prompt.append(" [yes/no]: ");

   std::string answer;
   if (!ReadInputLine(prompt.c_str(), &answer))
   {
      WriteToTerminalUtf8("\n");
      return false;
   }
   TrimString(&answer);
   ToLowerCase(&answer);
   return (answer == "yes") || (answer == "y");
}

/**
 * Shell that owns interactive session
 */
static Shell *s_shell = nullptr;

/**
 * Set when next line is a continuation of incomplete input
 */
static bool s_continuation = false;

/**
 * Build input prompt. If escape character is not 0, terminal control sequences are enclosed in
 * pair of those characters as required by line editor to calculate prompt width.
 */
static std::string BuildPrompt(char escape)
{
   std::string text = s_continuation ? std::string("...") : s_shell->getPrompt();
   if (g_plainOutput)
      return text + "> ";

   std::string prompt;
   if (escape != 0)
      prompt.push_back(escape);
   prompt.append("\x1b[36;1m");
   if (escape != 0)
      prompt.push_back(escape);
   prompt.append(text).append(">");
   if (escape != 0)
      prompt.push_back(escape);
   prompt.append("\x1b[0m");
   if (escape != 0)
      prompt.push_back(escape);
   prompt.append(" ");
   return prompt;
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
   static std::string prompt;
#ifdef EL_PROMPT_ESC
   prompt = BuildPrompt('\1');
#else
   prompt = s_continuation ? std::string("...> ") : s_shell->getPrompt() + "> ";
#endif
   return const_cast<char*>(prompt.c_str());
}

/**
 * Complete command or object path at cursor position
 */
static unsigned char CompleteInput(EditLine *el, int ch)
{
   if (s_continuation)
      return CC_NORM;

   const LineInfo *lineInfo = el_line(el);
   std::string input(lineInfo->buffer, lineInfo->cursor - lineInfo->buffer);

   std::string completion;
   std::vector<std::string> candidates;
   s_shell->complete(input, &completion, &candidates);

   if (!completion.empty())
      return (el_insertstr(el, completion.c_str()) == -1) ? CC_ERROR : CC_REFRESH;

   if (candidates.empty())
      return CC_ERROR;

   // Show all candidates
   std::string text("\n");
   for(size_t i = 0; i < candidates.size(); i++)
      text.append("  ").append(candidates[i]).append("\n");
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

   s_editLine = el_init("nxshell", stdin, stdout, stderr);
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
   el_set(s_editLine, EL_ADDFN, "nxshell-complete", "Complete command or object path", CompleteInput);
   el_set(s_editLine, EL_BIND, "^I", "nxshell-complete", nullptr);
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
   while(!line->empty() && ((line->back() == '\n') || (line->back() == '\r')))
      line->pop_back();
   return true;
#else
   return ReadInputLine(BuildPrompt(0).c_str(), line);
#endif
}

/**
 * Show welcome message
 */
static void ShowWelcome(const Shell *shell)
{
   bool assistantMode = (shell->getMode() == ShellMode::AI);
   std::string text("\n");
   AppendHighlightedText(&text, "34;1", assistantMode ? "NetXMS AI Assistant" : "NetXMS Shell");
   text.append("\n");
   AppendHighlightedText(&text, "90", "Connected to ");
   AppendHighlightedText(&text, "90", shell->getClient()->getServerUrl());
   text.append("\n");
   AppendHighlightedText(&text, "90", assistantMode ?
      "Type your questions or commands. Use /help for list of available commands." :
      "Use help for list of available commands.");
   text.append("\n\n");
   WriteToTerminalUtf8(text.c_str());
}

/**
 * Run interactive session
 */
int RunInteractiveSession(Shell *shell)
{
   s_shell = shell;
   SetInterruptHandler(shell->getClient());
   InitializeLineEditor();
   ShowWelcome(shell);

   std::string input;
   while(!shell->isExitRequested())
   {
      InterlockedAnd(&s_interrupted, 0);
      shell->getClient()->resetCancellation();
      s_continuation = !input.empty();

      std::string line;
      if (!ReadCommandLine(&line))
      {
         WriteToTerminalUtf8("\n");
         if (s_interrupted > 0)
         {
            input.clear();   // Input was interrupted by Ctrl+C, start new line
            continue;
         }
         if (!input.empty())
         {
            input.clear();   // End of input (Ctrl+D) cancels incomplete input
            continue;
         }
         shell->leaveMode();
         continue;
      }

      if (input.empty())
      {
         TrimString(&line);
         if (line.empty())
            continue;
         input = line;
      }
      else
      {
         input.append("\n").append(line);
      }

      if (!shell->isInputComplete(input))
         continue;

      AddToHistory(input.c_str());
      shell->execute(input);
      input.clear();
   }

   ProgressIndicatorStop();
   ShutdownLineEditor();
   s_activeClient = nullptr;
   s_shell = nullptr;
   PrintStatus("Goodbye!");
   return 0;
}
