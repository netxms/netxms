/**
 * NXCP message builder for embedded systems
 *
 * Copyright (c) 2014 Raden Solutions
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Configuration
 */
#define MAX_MSG_SIZE        512
#define KEEP_MESSAGE_ID     1
#define ALLOW_STRING_FIELDS 1
#define ALLOW_INT64_FIELDS  0
#define ALLOW_FLOAT_FIELDS  0
#define ALLOW_BINARY_FIELDS 0

/**
 * Data types
 */
#define CSCP_DT_INTEGER    0
#define CSCP_DT_STRING     1
#define CSCP_DT_INT64      2
#define CSCP_DT_INT16      3
#define CSCP_DT_BINARY     4
#define CSCP_DT_FLOAT      5

/**
 * Message flags
 */
#define MF_BINARY          0x0001
#define MF_END_OF_FILE     0x0002
#define MF_DONT_ENCRYPT    0x0004
#define MF_END_OF_SEQUENCE 0x0008
#define MF_REVERSE_ORDER   0x0010
#define MF_CONTROL         0x0020


#pragma pack(1)

/**
 * Data field structure
 */
typedef struct
{
   uint32_t id;      // Variable identifier
   char type;         // Data type
   char padding;      // Padding
   uint16_t wInt16;
   union
   {
      uint32_t dwInteger;
      uint64_t qwInt64;
      double dFloat;
      struct
      {
         uint32_t len;
         uint16_t value[1];
      } string;
   } data;
} NXCP_DF;

#define df_int16  wInt16
#define df_int32  data.dwInteger
#define df_int64  data.qwInt64
#define df_real   data.dFloat
#define df_string data.string

/**
 * Message structure
 */
typedef struct
{
   uint16_t code;      // Message (command) code
   uint16_t flags;     // Message flags
   uint32_t size;      // Message size (including header) in bytes
   uint32_t id;        // Unique message identifier
   uint32_t numFields; // Number of fields in message
} NXCP_MESSAGE_HEADER;

#pragma pack()

/**
 * NXCP message buffer
 */
char nxcp_message[MAX_MSG_SIZE] = "\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

#if KEEP_MESSAGE_ID
static uint32_t nxcp_message_id = 1;
#endif
static char *curr;

/**
 * Initialize message
 */
void nxcp_init_message()
{
   ((NXCP_MESSAGE_HEADER *)nxcp_message)->numFields = 0;
   curr = nxcp_message + 16;
}

/**
 * Add INT16 field
 */
void nxcp_add_int16(uint32_t id, uint16_t value)
{
   NXCP_DF *df = (NXCP_DF *)curr;
   df->id = htonl(id);
   df->type = CSCP_DT_INT16;
   df->df_int16 = htons(value);
   curr += 8;
   ((NXCP_MESSAGE_HEADER *)nxcp_message)->numFields++;
}

/**
 * Add INT32 field
 */
void nxcp_add_int32(uint32_t id, uint32_t value)
{
   NXCP_DF *df = (NXCP_DF *)curr;
   df->id = htonl(id);
   df->type = CSCP_DT_INTEGER;
   df->df_int32 = htonl(value);
   curr += 16;
   ((NXCP_MESSAGE_HEADER *)nxcp_message)->numFields++;
}

#if ALLOW_INT64_FIELDS

/**
 * Add INT64 field
 */
void nxcp_add_int64(uint32_t id, uint64_t value)
{
   NXCP_DF *df = (NXCP_DF *)curr;
   df->id = htonl(id);
   df->type = CSCP_DT_INT64;
   df->df_int64 = htonll(value);
   curr += 16;
   ((NXCP_MESSAGE_HEADER *)nxcp_message)->numFields++;
}

#endif

#if ALLOW_STRING_FIELDS

/**
 * Add string field
 */
void nxcp_add_string(uint32_t id, const char *value)
{
   uint32_t l;
   const char *p;
   NXCP_DF *df = (NXCP_DF *)curr;
   df->id = htonl(id);
   df->type = CSCP_DT_STRING;
   for(l = 0, p = value, curr += 12; *p != 0; l += 2, p++)
   {
      *(curr++) = 0;
      *(curr++) = *p;
   }
   df->df_string.len = htonl(l);
   int padding = 8 - (l + 12) % 8;
   curr += (padding < 8) ? padding : 0;
   ((NXCP_MESSAGE_HEADER *)nxcp_message)->numFields++;
}

#endif

#if ALLOW_BINARY_FIELDS

/**
 * Add binary field
 */
void nxcp_add_binary(uint32_t id, const char *value, uint32_t len)
{
   NXCP_DF *df = (NXCP_DF *)curr;
   df->id = htonl(id);
   df->type = CSCP_DT_BINARY;
   df->df_string.len = htonl(len);
   curr += 12;
   memcpy(curr, value, len);
   curr += len;
   int padding = 8 - (len + 12) % 8;
   curr += (padding < 8) ? padding : 0;
   ((NXCP_MESSAGE_HEADER *)nxcp_message)->numFields++;
}

#endif

/**
 * Finalize message. Returns number of bytes to be sent from nxcp_message array.
 */
uint32_t nxcp_finalize()
{
   uint32_t l = (uint32_t)(curr - nxcp_message);
   ((NXCP_MESSAGE_HEADER *)nxcp_message)->size = htonl(l);
   ((NXCP_MESSAGE_HEADER *)nxcp_message)->numFields = htonl(((NXCP_MESSAGE_HEADER *)nxcp_message)->numFields);
#if KEEP_MESSAGE_ID
   ((NXCP_MESSAGE_HEADER *)nxcp_message)->id = htonl(nxcp_message_id++);
#endif
   return l;
}
 
#if 0

/**
 * Example
 */
void example()
{
   nxcp_init_message();
   nxcp_add_int16(1, 120);
   nxcp_add_string(2, "test message");
   uint32_t l = nxcp_finalize();
   /* now send l bytes from nxcp_message to the network */
   send(sock, nxcp_message, l, 0);
}

#endif
