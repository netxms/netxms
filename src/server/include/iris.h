/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: iris.h
**
**/

#ifndef _iris_h_
#define _iris_h_

#define AI_ASSISTANT_COMPONENT   L"AI-ASSISTANT"

/**
 * AI assistant function handler
 */
typedef std::string (*AssistantFunctionHandler)(json_t*, uint32_t);

/**
 * Register assistant function
 */
void NXCORE_EXPORTABLE RegisterAIAssistantFunction(const char *name, const char *description, const std::vector<std::pair<const char*, const char*>>& properties, AssistantFunctionHandler handler);

/**
 * Call AI assistant function (intended for MCP bridge)
 */
std::string NXCORE_EXPORTABLE CallAIAssistantFunction(const char *name, json_t *arguments, uint32_t userId);

/**
 * Add custom prompt
 */
void NXCORE_EXPORTABLE AddAIAssistantPrompt(const char *text);

/**
 * Clear chat history for given session
 */
uint32_t NXCORE_EXPORTABLE ClearAIAssistantChat(GenericClientSession *session);

/**
 * Process assistant request
 */
char NXCORE_EXPORTABLE *ProcessRequestToAIAssistant(const char *prompt, NetObj *context, GenericClientSession *session);

#endif
