/*
** NetXMS Application Connector Library
** Copyright (C) 2015 Raden Solutions
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files
** (the "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to permit
** persons to whom the Software is furnished to do so, subject to the 
** following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
** OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
** OTHER DEALINGS IN THE SOFTWARE.
**/

#ifndef _nxappc_internal_h_
#define _nxappc_internal_h_

/**
 * Message start indicator
 */
#define NXAPPC_MSG_START_INDICATOR        "NXAPPC\x7F"
#define NXAPPC_MSG_START_INDICATOR_LEN    7

/**
 * Command codes
 */
#define NXAPPC_CMD_REGISTER_COUNTER       0x00
#define NXAPPC_CMD_RESET_COUNTER          0x01
#define NXAPPC_CMD_SET_COUNTER_LONG       0x02
#define NXAPPC_CMD_SET_COUNTER_DOUBLE     0x03
#define NXAPPC_CMD_UPDATE_COUNTER_LONG    0x04
#define NXAPPC_CMD_UPDATE_COUNTER_DOUBLE  0x05
#define NXAPPC_CMD_SEND_EVENT             0x06
#define NXAPPC_CMD_SEND_DATA              0x07

#endif
