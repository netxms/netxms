/*
** NetXMS - Network Management System
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
** File: chat_bots.cpp
**
**/

#include "webapi.h"

/**
 * Map RCC from chat bot management functions to HTTP status code
 */
static int MapChatBotRCC(Context *context, uint32_t rcc)
{
   switch(rcc)
   {
      case RCC_SUCCESS:
         return 200;
      case RCC_NO_CHANNEL_NAME:
         context->setErrorResponse("Chat bot not found");
         return 404;
      case RCC_CHANNEL_ALREADY_EXIST:
         context->setErrorResponse("Chat bot with given name already exists");
         return 409;
      case RCC_INVALID_ARGUMENT:
      case RCC_INVALID_CHANNEL_NAME:
      case RCC_INVALID_DRIVER_NAME:
         context->setErrorResponse("Missing or invalid required fields");
         return 400;
      default:
         context->setErrorResponse("Internal server error");
         return 500;
   }
}

/**
 * Handler for /v1/chat-bots (GET)
 */
int H_ChatBots(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   json_t *output = GetChatBots(true);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for /v1/chat-bots/:bot-name (GET)
 */
int H_ChatBotDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *botName = context->getPlaceholderValue(L"bot-name");
   if (botName == nullptr)
      return 400;

   json_t *output = GetChatBotByName(botName, true);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for /v1/chat-bots (POST)
 */
int H_ChatBotCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   const char *name = json_object_get_string_utf8(request, "name", nullptr);
   uint32_t rcc = CreateChatBotFromJson(request);
   if (rcc != RCC_SUCCESS)
      return MapChatBotRCC(context, rcc);

   wchar_t nameW[MAX_OBJECT_NAME];
   utf8_to_wchar(name, -1, nameW, MAX_OBJECT_NAME);
   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Chat bot \"%s\" created via REST API", nameW);

   json_t *output = GetChatBotByName(nameW, true);
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Handler for /v1/chat-bots/:bot-name (PUT)
 */
int H_ChatBotUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *botName = context->getPlaceholderValue(L"bot-name");
   if (botName == nullptr)
      return 400;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   uint32_t rcc = UpdateChatBotFromJson(botName, request);
   if (rcc != RCC_SUCCESS)
      return MapChatBotRCC(context, rcc);

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Chat bot \"%s\" updated via REST API", botName);

   json_t *output = GetChatBotByName(botName, true);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for /v1/chat-bots/:bot-name (DELETE)
 */
int H_ChatBotDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *botName = context->getPlaceholderValue(L"bot-name");
   if (botName == nullptr)
      return 400;

   wchar_t name[MAX_OBJECT_NAME];
   wcslcpy(name, botName, MAX_OBJECT_NAME);

   uint32_t rcc = DeleteChatBot(name);
   if (rcc != RCC_SUCCESS)
      return MapChatBotRCC(context, rcc);

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Chat bot \"%s\" deleted via REST API", name);
   return 204;
}

/**
 * Handler for /v1/chat-bots/:bot-name/rename (POST)
 */
int H_ChatBotRename(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *botName = context->getPlaceholderValue(L"bot-name");
   if (botName == nullptr)
      return 400;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonNewName = json_object_get(request, "newName");
   if (!json_is_string(jsonNewName))
   {
      context->setErrorResponse("Missing or invalid required field (newName)");
      return 400;
   }

   wchar_t newName[MAX_OBJECT_NAME];
   utf8_to_wchar(json_string_value(jsonNewName), -1, newName, MAX_OBJECT_NAME);
   if (newName[0] == 0)
   {
      context->setErrorResponse("Missing or invalid required field (newName)");
      return 400;
   }

   wchar_t name[MAX_OBJECT_NAME];
   wcslcpy(name, botName, MAX_OBJECT_NAME);

   uint32_t rcc = RenameChatBot(name, newName);
   if (rcc != RCC_SUCCESS)
      return MapChatBotRCC(context, rcc);

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Chat bot \"%s\" renamed to \"%s\" via REST API", name, newName);

   json_t *output = GetChatBotByName(newName, true);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for /v1/chat-bot-drivers (GET)
 */
int H_ChatBotDrivers(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   json_t *output = GetChatBotDriversAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}
