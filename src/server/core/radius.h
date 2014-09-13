/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
**
** RADIUS client
** This code is based on uRadiusLib (C) Gary Wallis, 2006.
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
** File: radius.h
**
**/

#ifndef _RADIUS_H_
#define _RADIUS_H_

#define AUTH_VECTOR_LEN			16
#define AUTH_PASS_LEN			128
#define AUTH_STRING_LEN			254	/* 253 max + trailing zero */

typedef struct pw_auth_hdr {
	BYTE		code;
	BYTE		id;
	u_short		length;
	BYTE		vector[AUTH_VECTOR_LEN];
	BYTE		data[2];
} AUTH_HDR;

#define AUTH_HDR_LEN                20
#define CHAP_VALUE_LENGTH           16

#define PW_AUTH_UDP_PORT            1645
#define PW_ACCT_UDP_PORT            1646

#define VENDORPEC_USR               429
#define VENDORPEC_CISTRON           8246

#define PW_TYPE_STRING              0
#define PW_TYPE_INTEGER             1
#define PW_TYPE_IPADDR              2
#define PW_TYPE_DATE                3


#define PW_AUTHENTICATION_REQUEST   1
#define PW_AUTHENTICATION_ACK       2
#define PW_AUTHENTICATION_REJECT    3
#define PW_ACCOUNTING_REQUEST       4
#define PW_ACCOUNTING_RESPONSE		5
#define PW_ACCOUNTING_STATUS        6
#define PW_PASSWORD_REQUEST         7
#define PW_PASSWORD_ACK             8
#define PW_PASSWORD_REJECT          9
#define PW_ACCOUNTING_MESSAGE       10
#define PW_ACCESS_CHALLENGE         11
#define PW_STATUS_SERVER            12
#define PW_STATUS_CLIENT            13

#define PW_USER_NAME                1
#define PW_PASSWORD                 2
#define PW_CHAP_PASSWORD            3
#define PW_NAS_IP_ADDRESS           4
#define PW_NAS_PORT                 5
#define PW_SERVICE_TYPE             6
#define PW_FRAMED_PROTOCOL          7
#define PW_FRAMED_IP_ADDRESS        8
#define PW_FRAMED_IP_NETMASK        9
#define PW_FRAMED_ROUTING           10
#define PW_FILTER_ID                11
#define PW_FRAMED_MTU               12
#define PW_FRAMED_COMPRESSION       13
#define PW_LOGIN_IP_HOST            14
#define PW_LOGIN_SERVICE            15
#define PW_LOGIN_TCP_PORT           16
#define PW_OLD_PASSWORD             17
#define PW_REPLY_MESSAGE            18
#define PW_CALLBACK_NUMBER          19
#define PW_CALLBACK_ID              20
#define PW_EXPIRATION               21
#define PW_FRAMED_ROUTE             22
#define PW_FRAMED_IPXNET            23
#define PW_STATE                    24
#define PW_CLASS                    25
#define PW_VENDOR_SPECIFIC          26
#define PW_SESSION_TIMEOUT          27
#define PW_IDLE_TIMEOUT             28
#define PW_CALLED_STATION_ID        30
#define PW_CALLING_STATION_ID       31
#define PW_PROXY_STATE              33

#define PW_ACCT_STATUS_TYPE         40
#define PW_ACCT_DELAY_TIME          41
#define PW_ACCT_INPUT_OCTETS        42
#define PW_ACCT_OUTPUT_OCTETS       43
#define PW_ACCT_SESSION_ID          44
#define PW_ACCT_AUTHENTIC           45
#define PW_ACCT_SESSION_TIME        46
#define PW_ACCT_INPUT_PACKETS       47
#define PW_ACCT_OUTPUT_PACKETS		48

#define PW_CHAP_CHALLENGE           60
#define PW_NAS_PORT_TYPE            61
#define PW_PORT_LIMIT               62
#define PW_CONNECT_INFO             77

/* Vendor specific attributes */
#define PW_CISTRON_PROXIED_TO       ((VENDORPEC_CISTRON<<16)|11)

/* Server side attributes */
#define PW_FALL_THROUGH			500
#define PW_EXEC_PROGRAM			502
#define PW_EXEC_PROGRAM_WAIT		503

#define PW_AUTHTYPE			1000
#define PW_PREFIX			1003
#define PW_SUFFIX			1004
#define PW_GROUP			1005
#define PW_CRYPT_PASSWORD		1006
#define PW_CONNECT_RATE			1007
#define PW_USER_CATEGORY		1029
#define PW_GROUP_NAME			1030
#define PW_HUNTGROUP_NAME		1031
#define PW_SIMULTANEOUS_USE		1034
#define PW_STRIP_USERNAME		1035
#define PW_HINT				1040
#define PAM_AUTH_ATTR			1041
#define PW_LOGIN_TIME			1042
#define PW_REALM			1045
#define PW_CLIENT_IP_ADDRESS		1052

/*
 *	INTEGER TRANSLATIONS
 */

/*	USER TYPES	*/

#define	PW_LOGIN_USER			1
#define	PW_FRAMED_USER			2
#define	PW_DIALBACK_LOGIN_USER		3
#define	PW_DIALBACK_FRAMED_USER		4

/*	FRAMED PROTOCOLS	*/

#define	PW_PPP				1
#define	PW_SLIP				2

/*	FRAMED ROUTING VALUES	*/

#define	PW_NONE				0
#define	PW_BROADCAST			1
#define	PW_LISTEN			2
#define	PW_BROADCAST_LISTEN		3

/*	FRAMED COMPRESSION TYPES	*/

#define	PW_VAN_JACOBSEN_TCP_IP		1

/*	LOGIN SERVICES	*/
#define	PW_TELNET			0
#define	PW_RLOGIN			1
#define	PW_TCP_CLEAR			2
#define	PW_PORTMASTER			3

/*	AUTHENTICATION LEVEL	*/
#define PW_AUTHTYPE_LOCAL		0
#define PW_AUTHTYPE_SYSTEM		1
#define PW_AUTHTYPE_SECURID		2
#define PW_AUTHTYPE_CRYPT		3
#define PW_AUTHTYPE_REJECT		4
#define PW_AUTHTYPE_MYSQL		252
#define PW_AUTHTYPE_PAM			253
#define PW_AUTHTYPE_ACCEPT		254

/*	PORT TYPES		*/
#define PW_NAS_PORT_ASYNC		0
#define PW_NAS_PORT_SYNC		1
#define PW_NAS_PORT_ISDN		2
#define PW_NAS_PORT_ISDN_V120		3
#define PW_NAS_PORT_ISDN_V110		4

/*	STATUS TYPES	*/

#define PW_STATUS_START			1
#define PW_STATUS_STOP			2
#define PW_STATUS_ALIVE			3
#define PW_STATUS_ACCOUNTING_ON		7
#define PW_STATUS_ACCOUNTING_OFF    8


#define VENDOR(x) (x >> 16)


 /*
  *	This defines for tagged string attrs whether the tag
  *	is actually inserted or not...! Stupid IMHO, but
  *	that's what the draft says...
  */

#define TAG_VALID(x)   ((x) > 0 && (x) < 0x20)
#define TAG_VALID_ZERO(x)   ((x) >= 0 && (x) < 0x20)


/* 
 *	This defines a TAG_ANY, the value for the tag if
 *	a wildcard ('*') was specified in a check item.
 */

#define TAG_ANY                -128     /* SCHAR_MIN */


typedef struct attr_flags
{
	char			addport;	/* Add port to IP address */
	char			has_tag;	/* attribute allows tags */
	signed char	tag;
	char			encrypt;	/* encryption method */
	signed char	len_disp;	/* length displacement */
} ATTR_FLAGS;


typedef struct value_pair
{
	char			name[40];
	int			attribute;
	int			type;
	int			length; /* of strvalue */
	ATTR_FLAGS  flags;
	UINT32			lvalue;
	int			op;
	char			strvalue[AUTH_STRING_LEN];
	int			group;
	struct value_pair	*next;
} VALUE_PAIR;

typedef struct auth_req {
	UINT32			ipaddr;
	WORD			udp_port;
	BYTE			id;
	BYTE			code;
	BYTE			vector[16];
	BYTE			secret[16];
	BYTE			username[AUTH_STRING_LEN];
	VALUE_PAIR		*request;
	int			child_pid;	/* Process ID of child */
	UINT32			timestamp;
	BYTE			*data;		/* Raw received data */
	int			data_len;
	VALUE_PAIR		*proxy_pairs;
	/* Proxy support fields */
	char			realm[64];
	int			validated;	/* Already md5 checked */
	UINT32			server_ipaddr;
	UINT32			server_id;
	VALUE_PAIR		*server_reply;	/* Reply from other server */
	int			server_code;	/* Reply code from other srv */
	struct auth_req		*next;		/* Next active request */
} AUTH_REQ;

enum
{
  PW_OPERATOR_EQUAL = 0,	/* = */
  PW_OPERATOR_NOT_EQUAL,	/* != */
  PW_OPERATOR_LESS_THAN,	/* < */
  PW_OPERATOR_GREATER_THAN,	/* > */
  PW_OPERATOR_LESS_EQUAL,	/* <= */
  PW_OPERATOR_GREATER_EQUAL,	/* >= */
  PW_OPERATOR_SET,		/* := */
  PW_OPERATOR_ADD,		/* += */
  PW_OPERATOR_SUB,		/* -= */
} PW_OPERATORS;

#endif   /* _RADIUS_H_ */
