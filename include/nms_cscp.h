/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: nms_cscp.h
**
**/

#ifndef _nms_cscp_h_
#define _nms_cscp_h_


//
// Constants
//

#define SERVER_LISTEN_PORT       4701


//
// Data field structure
//

typedef struct
{
   char szName[15];     // Variable name
   BYTE bType;          // Data type
   union
   {
      struct
      {
         WORD wLen;
         char szValue[1];
      } string;
      DWORD dwInteger;
      __int64 qwInt64;
   } data;
} CSCP_DF;


//
// Message structure
//

typedef struct
{
   WORD wCode;    // Message (command) code
   WORD wSize;    // Message size (including header) in bytes
   DWORD dwId;    // Unique message identifier
   CSCP_DF df[1]; // Data fields
} CSCP_MESSAGE;


//
// Data types
//

#define DT_INTEGER   0
#define DT_STRING    1
#define DT_INT64     2


//
// Message (command) codes
//

#define CMD_LOGIN          1
#define CMD_LOGIN_RESP     2
#define CMD_KEEPALIVE      3
#define CMD_EVENT          4
#define CMD_GET_OBJECTS    5
#define CMD_OBJECT         6
#define CMD_DELETE_OBJECT  7
#define CMD_UPDATE_OBJECT  8


#endif   /* _nms_cscp_h_ */
