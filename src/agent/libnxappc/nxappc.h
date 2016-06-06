/*
** NetXMS Application Connector Library
** Copyright (C) 2015-2016 Raden Solutions
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

#ifndef _nxappc_h_
#define _nxappc_h_

#ifdef _WIN32
#ifdef LIBNXAPPC_EXPORTS
#define LIBNXAPPC_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXAPPC_EXPORTABLE __declspec(dllimport)
#endif
#else
#define LIBNXAPPC_EXPORTABLE
#endif

#define NXAPPC_SUCCESS  (0)
#define NXAPPC_FAIL     (-1)

#define NXAPPC_CMD_REGISTER_COUNTER       0x00
#define NXAPPC_CMD_RESET_COUNTER          0x01
#define NXAPPC_CMD_SET_COUNTER_LONG       0x02
#define NXAPPC_CMD_SET_COUNTER_DOUBLE     0x03
#define NXAPPC_CMD_UPDATE_COUNTER_LONG    0x04
#define NXAPPC_CMD_UPDATE_COUNTER_DOUBLE  0x05
#define NXAPPC_CMD_SEND_EVENT             0x06
#define NXAPPC_CMD_SEND_DATA              0x07

#ifdef __cplusplus
extern "C" {
#endif

int LIBNXAPPC_EXPORTABLE nxappc_open_channel(void);
int LIBNXAPPC_EXPORTABLE nxappc_reset_channel(void);
int LIBNXAPPC_EXPORTABLE nxappc_set_channel_name(const char *channel);

int LIBNXAPPC_EXPORTABLE nxappc_send_event(int code, const char *name, const char *format, ...);
int LIBNXAPPC_EXPORTABLE nxappc_send_data(void *data, int size);

int LIBNXAPPC_EXPORTABLE nxappc_register_counter(const char *counter, const char *instance);
int LIBNXAPPC_EXPORTABLE nxappc_update_counter_long(const char *counter, const char *instance, long diff);
int LIBNXAPPC_EXPORTABLE nxappc_update_counter_double(const char *counter, const char *instance, double diff);
int LIBNXAPPC_EXPORTABLE nxappc_reset_counter(const char *counter, const char *instance);
int LIBNXAPPC_EXPORTABLE nxappc_set_counter_long(const char *counter, const char *instance, long value);
int LIBNXAPPC_EXPORTABLE nxappc_set_counter_double(const char *counter, const char *instance, double value);

#ifdef __cplusplus
}
#endif

#endif
