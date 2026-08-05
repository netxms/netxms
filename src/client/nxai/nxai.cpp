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
** File: nxai.cpp
**
**/

#include "nxai.h"
#include <netxms_getopt.h>
#include <nxlibcurl.h>
#include <nxmarkdown.h>

#ifndef _WIN32
#include <termios.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxai)

/**
 * Plain output mode - no colors and no markdown formatting. Set if requested explicitly or if
 * standard output is redirected.
 */
bool g_plainOutput = false;

/**
 * Print message with given SGR attributes
 */
static void PrintMessage(const char *attributes, const char *prefix, const char *format, va_list args)
{
   char message[4096];
   vsnprintf(message, sizeof(message), format, args);

   std::string text;
   if (!g_plainOutput)
      text.append("\x1b[").append(attributes).append("m");
   if (prefix != nullptr)
      text.append(prefix);
   text.append(message);
   if (!g_plainOutput)
      text.append("\x1b[0m");
   text.append("\n");
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
 * Print error message. In plain output mode message is written to standard error stream, so that
 * it is not mixed with tool output when output is redirected.
 */
void PrintError(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   if (g_plainOutput)
   {
      char message[4096];
      vsnprintf(message, sizeof(message), format, args);
      fprintf(stderr, "Error: %s\n", message);
   }
   else
   {
      PrintMessage("31;1", "Error: ", format, args);
   }
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
 * Create context document for outgoing message. Returns nullptr if session has no context set.
 */
json_t *ChatSession::createContextDocument() const
{
   if ((objectId == 0) && (incidentId == 0))
      return nullptr;

   json_t *context = json_object();
   if (objectId != 0)
      json_object_set_new(context, "objectId", json_integer(objectId));
   if (incidentId != 0)
      json_object_set_new(context, "incidentId", json_integer(incidentId));
   return context;
}

/**
 * Send message to assistant, answer questions asked during processing, and render response
 */
bool ChatSession::sendMessage(const char *message)
{
   json_t *context = createContextDocument();
   ChatResponse response;
   bool success = client->sendMessage(chatId, message, context, &response, progressCallback);
   json_decref(context);

   while(success && (response.question.id != 0))
   {
      ProgressIndicatorStop();

      bool positive = false;
      int selectedOption = -1;
      if (interactive)
      {
         // Question is declined if user cancels input
         PromptForAnswer(response.question, &positive, &selectedOption);
      }
      else
      {
         PrintWarning("assistant asked a question that requires interactive session, declining");
         PrintStatus("%s", response.question.text.c_str());
      }

      success = client->answerQuestion(chatId, response.question.id, positive, selectedOption) &&
                client->waitForResponse(chatId, &response, progressCallback);
   }

   ProgressIndicatorStop();

   if (!success)
   {
      PrintError("%s", client->getErrorText());
      return false;
   }

   if (!response.text.empty())
      RenderResponse(response.text.c_str());
   return true;
}

/**
 * Command line options
 */
static struct option s_longOptions[] =
{
   { (char *)"clear-session",   no_argument,       nullptr, 'C' },
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

#define SHORT_OPTIONS "hi:n:o:p:s:u:V"

/**
 * Show version information
 */
static void ShowVersion()
{
   _tprintf(
      _T("NetXMS AI Assistant  Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG _T("\n")
      _T("Copyright (c) 2025-2026 Raden Solutions\n\n"));
}

/**
 * Show usage info
 */
static void ShowUsage()
{
   ShowVersion();
   _tprintf(
      _T("Usage: nxai [OPTIONS] [message]\n")
      _T("\n")
      _T("If message is given on command line or provided on standard input, it is sent to assistant\n")
      _T("and tool exits after printing response. Otherwise interactive session is started.\n")
      _T("\n")
      _T("Options:\n")
      _T("  -h, --help                Display this help message.\n")
      _T("  -i, --incident <id>       Set incident with given ID as conversation context.\n")
      _T("  -n, --node <name>         Set node with given name as conversation context.\n")
      _T("  -o, --object <id>         Set object with given ID as conversation context.\n")
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
 * Authenticate on server. User name and password are requested from user if not provided.
 */
static bool Authenticate(WebApiClient *client, const char *user, const char *password)
{
   std::string userBuffer;
   if (*user == 0)
   {
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
   if (*password == 0)
   {
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
 * Entry point
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true, true);

   const char *optServer = "";
   const char *optUser = "";
   const char *optPassword = "";
   const char *optNode = "";
   uint32_t optObjectId = 0;
   uint32_t optIncidentId = 0;
   bool optPlain = false;
   bool optVerifySsl = true;
   bool optSaveSession = true;
   bool optClearSession = false;

   opterr = 0;
   int c;
   while((c = getopt_long(argc, argv, SHORT_OPTIONS, s_longOptions, nullptr)) != -1)
   {
      switch(c)
      {
         case 'C':   // clear session
            optClearSession = true;
            break;
         case 'h':   // help
            ShowUsage();
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
         case 'n':   // node context
            optNode = optarg;
            break;
         case 'o':   // object context
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
            ShowVersion();
            return 0;
         case '?':
            ShowUsage();
            return 1;
      }
   }

   g_plainOutput = optPlain || (_isatty(_fileno(stdout)) == 0);

   if ((optNode[0] != 0) && ((optObjectId != 0) || (optIncidentId != 0)))
   {
      PrintError("only one context source can be used");
      return 1;
   }
   if ((optObjectId != 0) && (optIncidentId != 0))
   {
      PrintError("only one context source can be used");
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
   if (*optPassword == 0)
   {
      const char *password = getenv("NETXMS_PASSWORD");
      optPassword = (password != nullptr) ? password : "";
   }

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

   // Reuse saved session if possible, otherwise authenticate
   bool savedSession = false;
   if (optSaveSession)
   {
      std::string token;
      if (LoadSessionToken(client.getServerUrl(), &token))
      {
         client.setToken(token.c_str());
         savedSession = true;
         PrintStatus("Using saved session for %s", client.getServerUrl());
      }
   }

   if (!savedSession)
   {
      if (!Authenticate(&client, optUser, optPassword))
         return 2;
      if (optSaveSession)
         SaveSessionToken(client.getServerUrl(), client.getToken());
   }

   ChatSession session(&client, client.getServerUrl());
   session.objectId = optObjectId;
   session.incidentId = optIncidentId;

   if (*optNode != 0)
   {
      ObjectInfo object;
      if (!client.findObject(optNode, &object))
      {
         // Saved session may be rejected by server, in that case authenticate and try again
         if (!savedSession || !client.isAuthenticationError())
         {
            PrintError("%s", client.getErrorText());
            return 3;
         }

         ClearSessionToken(client.getServerUrl());
         client.setToken(nullptr);
         savedSession = false;
         if (!Authenticate(&client, optUser, optPassword))
            return 2;
         if (optSaveSession)
            SaveSessionToken(client.getServerUrl(), client.getToken());

         if (!client.findObject(optNode, &object))
         {
            PrintError("%s", client.getErrorText());
            return 3;
         }
      }

      if (object.id == 0)
      {
         PrintError("object \"%s\" not found", optNode);
         return 3;
      }
      session.objectId = object.id;
      PrintStatus("Conversation context set to %s [%u]", object.name.c_str(), object.id);
   }

   if (!client.createChat(session.incidentId, session.objectId, &session.chatId))
   {
      // Saved session may be rejected by server, in that case authenticate and try again
      if (!savedSession || !client.isAuthenticationError())
      {
         PrintError("%s", client.getErrorText());
         return 3;
      }

      ClearSessionToken(client.getServerUrl());
      client.setToken(nullptr);
      if (!Authenticate(&client, optUser, optPassword))
         return 2;
      if (optSaveSession)
         SaveSessionToken(client.getServerUrl(), client.getToken());

      if (!client.createChat(session.incidentId, session.objectId, &session.chatId))
      {
         PrintError("%s", client.getErrorText());
         return 3;
      }
   }

   // Message given on command line or provided on standard input is processed in non-interactive mode
   std::string message;
   for(int i = optind; i < argc; i++)
   {
      if (!message.empty())
         message.append(" ");
      message.append(argv[i]);
   }
   if (message.empty() && (_isatty(_fileno(stdin)) == 0))
      message = ReadStandardInput();
   TrimString(&message);

   // Assistant questions can be answered only if input is read from terminal
   session.interactive = (_isatty(_fileno(stdin)) != 0);

   // Progress indicator can be shown only if output is not redirected
   if (!g_plainOutput)
      session.progressCallback = ProgressIndicatorUpdate;

   int rc;
   if (!message.empty())
   {
      rc = session.sendMessage(message.c_str()) ? 0 : 4;
   }
   else if (session.interactive)
   {
      rc = RunChatSession(&session);
   }
   else
   {
      PrintError("message is empty");
      rc = 1;
   }

   client.deleteChat(session.chatId);
   return rc;
}
