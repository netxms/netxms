/*
** NetXMS - Network Management System
** NETCONF protocol support library
** Copyright (C) 2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: framing.cpp
**
**/

#include "libnxnetconf.h"

/**
 * Chunked framing decoder states
 */
enum ChunkDecoderState
{
   CHUNK_STATE_LF1 = 0,        // expecting LF starting chunk header
   CHUNK_STATE_HASH1 = 1,      // expecting '#' of chunk header
   CHUNK_STATE_SIZE_FIRST = 2, // expecting first digit of chunk size or '#' of end-of-chunks marker
   CHUNK_STATE_SIZE = 3,       // expecting next digit of chunk size or LF ending chunk header
   CHUNK_STATE_DATA = 4,       // reading chunk data
   CHUNK_STATE_END_LF = 5      // expecting LF completing end-of-chunks marker
};

/**
 * Message decoder constructor
 */
NETCONF_MessageDecoder::NETCONF_MessageDecoder(NetconfVersion version, size_t maxMessageSize) :
      m_input(8192), m_message(8192), m_completedMessages(0, 16, Ownership::True)
{
   m_version = version;
   m_state = CHUNK_STATE_LF1;
   m_chunkSize = 0;
   m_scanPos = 0;
   m_maxMessageSize = maxMessageSize;
   m_error = false;
}

/**
 * Change framing version. Should be called on message boundary (normally after
 * hello exchange, which always uses end-of-message framing). Any buffered bytes
 * not yet forming a complete message are re-processed with the new framing.
 */
void NETCONF_MessageDecoder::setVersion(NetconfVersion version)
{
   if (version == m_version)
      return;

   m_version = version;
   m_state = CHUNK_STATE_LF1;
   m_chunkSize = 0;
   m_scanPos = 0;
   m_message.clear();

   if ((m_input.size() > 0) && !m_error)
   {
      if (version == NetconfVersion::NETCONF_1_1)
         decodeChunkedFrames();
      else
         decodeEomFrames();
   }
}

/**
 * Store completed message
 */
void NETCONF_MessageDecoder::completeMessage(const void *data, size_t size)
{
   auto message = new ByteStream(size > 0 ? size : 1);
   message->write(data, size);
   m_completedMessages.add(message);
}

/**
 * Decode messages in end-of-message framing (delimited by ]]>]]>)
 */
void NETCONF_MessageDecoder::decodeEomFrames()
{
   while(true)
   {
      size_t size;
      const BYTE *data = m_input.buffer(&size);
      if (size < 6)
         break;

      const BYTE *p = static_cast<const BYTE*>(memmem(data + m_scanPos, size - m_scanPos, "]]>]]>", 6));
      if (p == nullptr)
      {
         m_scanPos = size - 5;
         break;
      }

      size_t messageSize = p - data;
      completeMessage(data, messageSize);
      m_input.truncateLeft(messageSize + 6);
      m_scanPos = 0;
   }

   if (m_input.size() > m_maxMessageSize)
      m_error = true;
}

/**
 * Decode messages in chunked framing (RFC 6242 section 4.2). All buffered input
 * is consumed by the state machine.
 */
void NETCONF_MessageDecoder::decodeChunkedFrames()
{
   size_t size;
   const BYTE *data = m_input.buffer(&size);
   size_t pos = 0;
   while((pos < size) && !m_error)
   {
      char c = static_cast<char>(data[pos]);
      switch(m_state)
      {
         case CHUNK_STATE_LF1:
            if (c == '\n')
            {
               m_state = CHUNK_STATE_HASH1;
               pos++;
            }
            else
            {
               m_error = true;
            }
            break;
         case CHUNK_STATE_HASH1:
            if (c == '#')
            {
               m_state = CHUNK_STATE_SIZE_FIRST;
               pos++;
            }
            else
            {
               m_error = true;
            }
            break;
         case CHUNK_STATE_SIZE_FIRST:
            if ((c >= '1') && (c <= '9'))
            {
               m_chunkSize = c - '0';
               m_state = CHUNK_STATE_SIZE;
               pos++;
            }
            else if (c == '#')
            {
               m_state = CHUNK_STATE_END_LF;
               pos++;
            }
            else
            {
               m_error = true;
            }
            break;
         case CHUNK_STATE_SIZE:
            if ((c >= '0') && (c <= '9'))
            {
               m_chunkSize = m_chunkSize * 10 + (c - '0');
               if (m_chunkSize > 0xFFFFFFFF)
                  m_error = true;
               else
                  pos++;
            }
            else if (c == '\n')
            {
               if (m_message.size() + m_chunkSize > m_maxMessageSize)
               {
                  m_error = true;
               }
               else
               {
                  m_state = CHUNK_STATE_DATA;
                  pos++;
               }
            }
            else
            {
               m_error = true;
            }
            break;
         case CHUNK_STATE_DATA:
         {
            size_t count = std::min(static_cast<size_t>(m_chunkSize), size - pos);
            m_message.write(data + pos, count);
            pos += count;
            m_chunkSize -= count;
            if (m_chunkSize == 0)
               m_state = CHUNK_STATE_LF1;
            break;
         }
         case CHUNK_STATE_END_LF:
            if (c == '\n')
            {
               completeMessage(m_message.buffer(), m_message.size());
               m_message.clear();
               m_state = CHUNK_STATE_LF1;
               pos++;
            }
            else
            {
               m_error = true;
            }
            break;
      }
   }

   if (m_error)
      nxlog_debug_tag(DEBUG_TAG_NETCONF, 6, _T("NETCONF_MessageDecoder: chunked framing protocol violation (state=%d)"), m_state);

   m_input.clear();
}

/**
 * Feed raw bytes received from transport into decoder
 */
void NETCONF_MessageDecoder::feed(const void *data, size_t size)
{
   if (m_error || (size == 0))
      return;

   m_input.write(data, size);
   if (m_version == NetconfVersion::NETCONF_1_1)
      decodeChunkedFrames();
   else
      decodeEomFrames();
}

/**
 * Read next complete message from decoder. Returns dynamically allocated
 * null-terminated message text (caller is responsible for calling MemFree on it)
 * or nullptr if no complete message is available.
 */
char *NETCONF_MessageDecoder::readMessage(size_t *size)
{
   if (m_completedMessages.isEmpty())
      return nullptr;

   ByteStream *message = m_completedMessages.get(0);
   size_t messageSize = message->size();
   char *text = MemAllocArrayNoInit<char>(messageSize + 1);
   memcpy(text, message->buffer(), messageSize);
   text[messageSize] = 0;
   m_completedMessages.remove(0);

   if (size != nullptr)
      *size = messageSize;
   return text;
}
