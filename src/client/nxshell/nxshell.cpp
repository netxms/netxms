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
** File: nxshell.cpp
**
**/

#include "nxshell.h"
#include <netxms_getopt.h>
#include <nxlibcurl.h>
#include <nxmarkdown.h>

#ifndef _WIN32
#include <termios.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxshell)

/**
 * Plain output mode - no colors and no markdown formatting. Set if requested explicitly or if
 * standard output is redirected.
 */
bool g_plainOutput = false;

/**
 * Print message with given SGR attributes. In plain output mode message is written to standard error
 * stream, so that it is not mixed with tool output when output is redirected.
 */
static void PrintMessage(const char *attributes, const char *prefix, const char *format, va_list args)
{
   char message[4096];
   vsnprintf(message, sizeof(message), format, args);

   if (g_plainOutput)
   {
      fflush(stdout);
      fprintf(stderr, "%s%s\n", (prefix != nullptr) ? prefix : "", message);
      return;
   }

   std::string text("\x1b[");
   text.append(attributes).append("m");
   if (prefix != nullptr)
      text.append(prefix);
   text.append(message);
   text.append("\x1b[0m\n");
   WriteToTerminalUtf8(text.c_str());
}

/**
 * Print status message
 */
void PrintStatus(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   PrintMessage("90", nullptr, format, args);
   va_end(args);
}

/**
 * Print success message
 */
void PrintSuccess(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   PrintMessage("32;1", nullptr, format, args);
   va_end(args);
}

/**
 * Print warning message
 */
void PrintWarning(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   PrintMessage("33;1", "Warning: ", format, args);
   va_end(args);
}

/**
 * Print error message
 */
void PrintError(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   PrintMessage("31;1", "Error: ", format, args);
   va_end(args);
}

/**
 * Render assistant response
 */
void RenderResponse(const char *text)
{
   char *renderedText = g_plainOutput ? MarkdownToPlainText(text) : MarkdownToTerminal(text);
   if (renderedText == nullptr)
      return;

   WriteToTerminalUtf8("\n");
   WriteToTerminalUtf8(renderedText);
   WriteToTerminalUtf8("\n");
   MemFree(renderedText);
}

/**
 * Write text received from server to standard output. Terminal control sequences are removed in plain output mode.
 */
void WriteOutput(const char *text)
{
   if (g_plainOutput)
      WriteToTerminalUtf8(StripTerminalSequences(text).c_str());
   else
      WriteToTerminalUtf8(text);
   fflush(stdout);
}

/**
 * Append text with given SGR attributes. Attributes are ignored in plain output mode.
 */
void AppendHighlightedText(std::string *output, const char *attributes, const char *text)
{
   if (!g_plainOutput)
      output->append("\x1b[").append(attributes).append("m");
   output->append(text);
   if (!g_plainOutput)
      output->append("\x1b[0m");
}

/**
 * Command line options
 */
static struct option s_longOptions[] =
{
   { (char *)"clear-session",   no_argument,       nullptr, 'C' },
   { (char *)"command",         required_argument, nullptr, 'c' },
   { (char *)"help",            no_argument,       nullptr, 'h' },
   { (char *)"incident",        required_argument, nullptr, 'i' },
   { (char *)"no-save-session", no_argument,       nullptr, 'S' },
   { (char *)"no-verify-ssl",   no_argument,       nullptr, 'k' },
   { (char *)"node",            required_argument, nullptr, 'n' },
   { (char *)"object",          required_argument, nullptr, 'o' },
   { (char *)"password",        required_argument, nullptr, 'p' },
   { (char *)"plain",           no_argument,       nullptr, 'l' },
   { (char *)"server",          required_argument, nullptr, 's' },
   { (char *)"user",            required_argument, nullptr, 'u' },
   { (char *)"version",         no_argument,       nullptr, 'V' },
   { nullptr, 0, nullptr, 0 }
};

#define SHORT_OPTIONS "c:hi:n:o:p:s:u:V"

/**
 * Show version information
 */
static void ShowVersion(bool assistantMode)
{
   _tprintf(
      _T("NetXMS %s  Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG _T("\n")
      _T("Copyright (c) 2025-2026 Raden Solutions\n\n"), assistantMode ? _T("AI Assistant") : _T("Shell"));
}

/**
 * Show usage info
 */
static void ShowUsage(bool assistantMode)
{
   ShowVersion(assistantMode);
   if (assistantMode)
   {
      _tprintf(
         _T("Usage: nxai [OPTIONS] [message]\n")
         _T("\n")
         _T("If message is given on command line or provided on standard input, it is sent to assistant\n")
         _T("and tool exits after printing response. Otherwise interactive session is started.\n")
         _T("\n")
         _T("Options:\n"));
   }
   else
   {
      _tprintf(
         _T("Usage: nxshell [OPTIONS] [file]\n")
         _T("\n")
         _T("Commands are read from given file, or from standard input if it is not a terminal, and\n")
         _T("executed until first failure. Otherwise interactive session is started.\n")
         _T("\n")
         _T("Options:\n")
         _T("  -c, --command <line>      Execute given line and exit (can be used multiple times).\n"));
   }
   _tprintf(
      _T("  -h, --help                Display this help message.\n")
      _T("  -i, --incident <id>       Set incident with given ID as AI assistant conversation context.\n")
      _T("  -n, --node <name>         Set object with given name as current object.\n")
      _T("  -o, --object <id>         Set object with given ID as current object.\n")
      _T("  -p, --password <password> Password for authentication.\n")
      _T("  -s, --server <server>     Server host name or URL (for example netxms.local or\n")
      _T("                            https://netxms.local:8443).\n")
      _T("  -u, --user <user>         User name for authentication.\n")
      _T("  -V, --version             Display version information.\n")
      _T("      --clear-session       Delete saved session for server and exit.\n")
      _T("      --no-save-session     Do not save session token for reuse.\n")
      _T("      --no-verify-ssl       Do not verify server SSL certificate.\n")
      _T("      --plain               Force plain text output without colors and formatting.\n")
      _T("\n")
      _T("Environment variables NETXMS_SERVER, NETXMS_USER, and NETXMS_PASSWORD are used as\n")
      _T("defaults for options -s, -u, and -p.\n\n"));
}

/**
 * Read line from standard input. Returns false on end of input.
 */
bool ReadInputLine(const char *prompt, std::string *line)
{
   WriteToTerminalUtf8(prompt);
   fflush(stdout);

   char buffer[1024];
   if (fgets(buffer, sizeof(buffer), stdin) == nullptr)
   {
      // Clear end of file indicator, so that input can be read again after user pressed Ctrl+D
      clearerr(stdin);
      return false;
   }

   line->assign(buffer);
   while(!line->empty() && ((line->back() == '\n') || (line->back() == '\r')))
      line->pop_back();
   return true;
}

/**
 * Read password from terminal with echo turned off. Password is read as byte stream, because wide
 * character input functions (used by ReadPassword in libnetxms) cannot be mixed with byte oriented
 * input used elsewhere in this tool.
 */
static bool ReadPasswordFromTerminal(const char *prompt, char *buffer, size_t size)
{
   WriteToTerminalUtf8(prompt);
   fflush(stdout);

   bool success;
#ifdef _WIN32
   HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
   DWORD mode;
   if (GetConsoleMode(stdinHandle, &mode))
   {
      SetConsoleMode(stdinHandle, mode & ~ENABLE_ECHO_INPUT);
      WCHAR wideText[MAX_PASSWORD];
      DWORD chars = 0;
      success = (ReadConsoleW(stdinHandle, wideText, MAX_PASSWORD - 1, &chars, nullptr) != 0);
      SetConsoleMode(stdinHandle, mode);
      if (success)
      {
         wideText[chars] = 0;
         size_t bytes = wchar_to_utf8(wideText, -1, buffer, size - 1);
         buffer[bytes] = 0;
      }
   }
   else
   {
      success = (fgets(buffer, static_cast<int>(size), stdin) != nullptr);
   }
#else
   struct termios savedAttributes;
   bool echoDisabled = (tcgetattr(fileno(stdin), &savedAttributes) == 0);
   if (echoDisabled)
   {
      struct termios attributes = savedAttributes;
      attributes.c_lflag &= ~ECHO;
      echoDisabled = (tcsetattr(fileno(stdin), TCSAFLUSH, &attributes) == 0);
   }

   success = (fgets(buffer, static_cast<int>(size), stdin) != nullptr);

   if (echoDisabled)
      tcsetattr(fileno(stdin), TCSAFLUSH, &savedAttributes);
#endif

   WriteToTerminalUtf8("\n");

   if (!success)
      return false;

   char *eol = strpbrk(buffer, "\r\n");
   if (eol != nullptr)
      *eol = 0;
   return true;
}

/**
 * Read all data from standard input
 */
static std::string ReadStandardInput()
{
   std::string text;
   char buffer[4096];
   size_t bytes;
   while((bytes = fread(buffer, 1, sizeof(buffer), stdin)) > 0)
      text.append(buffer, bytes);
   return text;
}

/**
 * Authenticate on server. User name and password are requested from user if not provided and input is read
 * from terminal. Password can be nullptr, which means that it was not provided at all - empty string is a
 * valid password and does not cause a prompt.
 */
static bool Authenticate(WebApiClient *client, const char *user, const char *password)
{
   bool terminalInput = (_isatty(_fileno(stdin)) != 0);

   std::string userBuffer;
   if (*user == 0)
   {
      if (!terminalInput)
      {
         PrintError("user name is not specified (use -u option or NETXMS_USER environment variable)");
         return false;
      }
      char prompt[256];
      snprintf(prompt, sizeof(prompt), "User name for %s: ", client->getServerUrl());
      if (!ReadInputLine(prompt, &userBuffer) || userBuffer.empty())
      {
         PrintError("user name not provided");
         return false;
      }
      user = userBuffer.c_str();
   }

   char passwordBuffer[MAX_PASSWORD];
   if (password == nullptr)
   {
      if (!terminalInput)
      {
         PrintError("password is not specified (use -p option or NETXMS_PASSWORD environment variable)");
         return false;
      }
      char prompt[256];
      snprintf(prompt, sizeof(prompt), "Password for %s@%s: ", user, client->getServerUrl());
      if (!ReadPasswordFromTerminal(prompt, passwordBuffer, sizeof(passwordBuffer)))
      {
         PrintError("password not provided");
         return false;
      }
      password = passwordBuffer;
   }

   PrintStatus("Connecting to %s...", client->getServerUrl());
   bool success = client->login(user, password);
   memset(passwordBuffer, 0, sizeof(passwordBuffer));
   if (!success)
      PrintError("%s", client->getErrorText());
   return success;
}

/**
 * Check if tool was started under AI assistant client name
 */
static bool IsAssistantCommandName(const char *path)
{
   const char *name = path;
   for(const char *p = path; *p != 0; p++)
   {
      if ((*p == '/') || (*p == '\\'))
         name = p + 1;
   }
   return !strnicmp(name, "nxai", 4) && ((name[4] == 0) || (name[4] == '.'));
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true, true);

   // Tool started as "nxai" works as AI assistant client
   bool assistantMode = IsAssistantCommandName(argv[0]);

   const char *optServer = "";
   const char *optUser = "";
   const char *optPassword = nullptr;   // nullptr means that password was not provided, empty string is a valid password
   const char *optNode = "";
   uint32_t optObjectId = 0;
   uint32_t optIncidentId = 0;
   bool optPlain = false;
   bool optVerifySsl = true;
   bool optSaveSession = true;
   bool optClearSession = false;
   std::vector<std::string> commands;

   opterr = 0;
   int c;
   while((c = getopt_long(argc, argv, SHORT_OPTIONS, s_longOptions, nullptr)) != -1)
   {
      switch(c)
      {
         case 'C':   // clear session
            optClearSession = true;
            break;
         case 'c':   // command
            if (assistantMode)
            {
               ShowUsage(assistantMode);
               return 1;
            }
            commands.push_back(optarg);
            break;
         case 'h':   // help
            ShowUsage(assistantMode);
            return 0;
         case 'i':   // incident context
            optIncidentId = strtoul(optarg, nullptr, 0);
            if (optIncidentId == 0)
            {
               PrintError("invalid incident ID \"%s\"", optarg);
               return 1;
            }
            break;
         case 'k':   // no SSL verification
            optVerifySsl = false;
            break;
         case 'l':   // plain output
            optPlain = true;
            break;
         case 'n':   // current object by name
            optNode = optarg;
            break;
         case 'o':   // current object by ID
            optObjectId = strtoul(optarg, nullptr, 0);
            if (optObjectId == 0)
            {
               PrintError("invalid object ID \"%s\"", optarg);
               return 1;
            }
            break;
         case 'p':   // password
            optPassword = optarg;
            break;
         case 'S':   // do not save session
            optSaveSession = false;
            break;
         case 's':   // server
            optServer = optarg;
            break;
         case 'u':   // user
            optUser = optarg;
            break;
         case 'V':   // version
            ShowVersion(assistantMode);
            return 0;
         case '?':
            ShowUsage(assistantMode);
            return 1;
      }
   }

   g_plainOutput = optPlain || (_isatty(_fileno(stdout)) == 0);

   if ((optNode[0] != 0) && (optObjectId != 0))
   {
      PrintError("options -n and -o cannot be used together");
      return 1;
   }
   if (assistantMode && (optIncidentId != 0) && ((optNode[0] != 0) || (optObjectId != 0)))
   {
      PrintError("only one context source can be used");
      return 1;
   }
   if (!assistantMode && ((argc - optind > 1) || ((argc > optind) && !commands.empty())))
   {
      PrintError("only one source of commands can be used");
      return 1;
   }

   if (*optServer == 0)
      optServer = getenv("NETXMS_SERVER");
   if ((optServer == nullptr) || (*optServer == 0))
   {
      PrintError("server is not specified (use -s option or NETXMS_SERVER environment variable)");
      return 1;
   }

   if (*optUser == 0)
   {
      const char *user = getenv("NETXMS_USER");
      optUser = (user != nullptr) ? user : "";
   }
   if (optPassword == nullptr)
      optPassword = getenv("NETXMS_PASSWORD");

   if (!InitializeLibCURL())
   {
      PrintError("cannot initialize cURL library");
      return 2;
   }

   WebApiClient client(optServer, optVerifySsl);

   if (optClearSession)
   {
      ClearSessionToken(client.getServerUrl());
      PrintStatus("Saved session for %s deleted", client.getServerUrl());
      return 0;
   }

   // Access token rejected by server is replaced by authenticating again
   client.setAuthenticator(
      [&client, optUser, optPassword, optSaveSession] () -> bool
      {
         ProgressIndicatorStop();
         ClearSessionToken(client.getServerUrl());
         client.setToken(nullptr);
         PrintStatus("Session is not valid, authentication required");
         if (!Authenticate(&client, optUser, optPassword))
            return false;
         if (optSaveSession)
            SaveSessionToken(client.getServerUrl(), client.getToken());
         return true;
      });

   // Reuse saved session if possible, otherwise authenticate
   std::string token;
   if (optSaveSession && LoadSessionToken(client.getServerUrl(), &token))
   {
      client.setToken(token.c_str());
      PrintStatus("Using saved session for %s", client.getServerUrl());
      if (!client.checkSession())
      {
         if (client.getHttpStatus() != 401)
            PrintError("%s", client.getErrorText());
         return 2;
      }
   }
   else
   {
      if (!Authenticate(&client, optUser, optPassword))
         return 2;
      if (optSaveSession)
         SaveSessionToken(client.getServerUrl(), client.getToken());
   }

   // Input source: message for assistant or commands given on command line, file, or standard input
   bool terminalInput = (_isatty(_fileno(stdin)) != 0);
   std::string message;
   if (assistantMode)
   {
      for(int i = optind; i < argc; i++)
      {
         if (!message.empty())
            message.append(" ");
         message.append(argv[i]);
      }
      if (message.empty() && !terminalInput)
         message = ReadStandardInput();
      TrimString(&message);
      if (message.empty() && !terminalInput)
      {
         PrintError("message is empty");
         return 1;
      }
   }
   const char *commandFile = (!assistantMode && (argc > optind)) ? argv[optind] : nullptr;
   bool interactive = terminalInput && message.empty() && commands.empty() && (commandFile == nullptr);

   int rc;
   {
      Shell shell(&client, assistantMode ? ShellMode::AI : ShellMode::SHELL, terminalInput, interactive);
      shell.setIncidentId(optIncidentId);

      // Progress indicator can be shown only if output is not redirected
      if (!g_plainOutput)
         shell.setProgressCallback(ProgressIndicatorUpdate);

      bool success = true;
      if (optObjectId != 0)
         success = shell.setLocationByObjectId(optObjectId);
      else if (*optNode != 0)
         success = shell.setLocationByObjectName(optNode);

      if (success)
      {
         shell.executeStartupFile();

         if (!message.empty())
         {
            success = shell.sendChatMessage(message.c_str());
         }
         else if (!commands.empty())
         {
            for(size_t i = 0; (i < commands.size()) && success && !shell.isExitRequested(); i++)
            {
               if (shell.isInputComplete(commands[i]))
               {
                  success = shell.execute(commands[i]);
               }
               else
               {
                  PrintError("incomplete command \"%s\"", commands[i].c_str());
                  success = false;
               }
            }
         }
         else if (commandFile != nullptr)
         {
            FILE *stream = fopen(commandFile, "r");
            if (stream != nullptr)
            {
               success = shell.executeStream(stream, commandFile, true);
               fclose(stream);
            }
            else
            {
               PrintError("cannot open file \"%s\" (%s)", commandFile, strerror(errno));
               success = false;
            }
         }
         else if (!terminalInput)
         {
            success = shell.executeStream(stdin, "standard input", true);
         }
         else
         {
            RunInteractiveSession(&shell);
         }
      }

      rc = success ? 0 : (client.isConnectionFailed() ? 2 : 1);
   }
   return rc;
}
