/* 
 ** NetXMS - Network Management System
 ** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
 ** File: radius.cpp
 **
 **/

#include "nxcore.h"
#include "radius.h"


//
// Add a pair at the end of a VALUE_PAIR list.
//

static void pairadd(VALUE_PAIR **first, VALUE_PAIR *newPair)
{
	VALUE_PAIR *i;

	if (*first == NULL)
	{
		*first = newPair;
		return;
	}
	for(i = *first; i->next; i = i->next)
		;
	i->next = newPair;
}


//
// Release the memory used by a list of attribute-value pairs.
//

static void pairfree(VALUE_PAIR *pair)
{
	VALUE_PAIR *next;

	while(pair != NULL)
	{
		next = pair->next;
		free(pair);
		pair = next;
	}
}


//
// Create a new pair.
//

static VALUE_PAIR *paircreate(int attr, int type, const char *pszName)
{
	VALUE_PAIR *vp;

	if ((vp = (VALUE_PAIR *)malloc(sizeof(VALUE_PAIR))) == NULL)
	{
		return NULL;
	}
	memset(vp, 0, sizeof(VALUE_PAIR));
	vp->attribute = attr;
	vp->op = PW_OPERATOR_EQUAL;
	vp->type = type;
	if (pszName != NULL)
	{
		strcpy(vp->name, pszName);
	}
	else
	{
		sprintf(vp->name, "Attr-%d", attr);
	}
	switch (vp->type)
	{
		case PW_TYPE_INTEGER:
		case PW_TYPE_IPADDR:
		case PW_TYPE_DATE:
			vp->length = 4;
			break;
		default:
			vp->length = 0;
			break;
	}

	return vp;
}


//
// Generate AUTH_VECTOR_LEN worth of random data.
//

static void random_vector(unsigned char *vector)
{
	static int did_srand = 0;
	unsigned int i;
	int n;

#if defined(__linux__) || defined(BSD)
	if (!did_srand)
	{
		/*
		 * Try to use /dev/urandom to seed the
		 * random generator. Not all *BSDs have it
		 * but it doesn't hurt to try.
		 */
		if ((n = open("/dev/urandom", O_RDONLY)) >= 0 &&
				read(n, (char *)&i, sizeof(i)) == sizeof(i))
		{
			srandom(i);
			did_srand = 1;
		}
		if (n >= 0)
		{
			close(n);
		}
	}

	if (!did_srand)
	{
		/*
		 * Use more traditional way to seed.
		 */
		srandom(time(NULL) + getpid());
		did_srand = 1;
	}

	for (i = 0; i < AUTH_VECTOR_LEN;)
	{
		n = random();
		memcpy(vector, &n, sizeof(int));
		vector += sizeof(int);
		i += sizeof(int);
	}
#else

#ifndef RAND_MAX
#  define RAND_MAX 32768
#endif
	/*
	 * Assume the system has neither /dev/urandom
	 * nor random()/srandom() but just the old
	 * rand() / srand() functions.
	 */
	if (!did_srand)
	{
		char garbage[8];
		i = (unsigned int)time(NULL) + getpid();
		for (n = 0; n < 8; n++)
		{
			i += (garbage[n] << i);
		}
		srand(i);
		did_srand = 1;
	}

	for (i = 0; i < AUTH_VECTOR_LEN;)
	{
		unsigned char c;
		/*
		 * Don't use the lower bits, also don't use
		 * parts > RAND_MAX since they are zero.
		 */
		n = rand() / (RAND_MAX / 256);
		c = n;
		memcpy(vector, &c, sizeof(c));
		vector += sizeof(c);
		i += sizeof(c);
	}
#endif
}


//
// Encode password.
//
// Assume that "pwd_out" points to a buffer of at least AUTH_PASS_LEN bytes.
//
// Returns new length.
//

static int rad_pwencode(const char *pwd_in, char *pwd_out, const char *secret, const char *vector)
{
	int passLen = (int)strlen(pwd_in);
	if (passLen > AUTH_PASS_LEN)
		passLen = AUTH_PASS_LEN;

	char passbuf[AUTH_PASS_LEN];
	memset(passbuf, 0, AUTH_PASS_LEN);
	memcpy(passbuf, pwd_in, passLen);

	int secretlen = (int)strlen(secret);
	if (secretlen + AUTH_VECTOR_LEN > 256)
	{
		secretlen = 256 - AUTH_VECTOR_LEN;
	}

	char key[256];
	strncpy(key, secret, sizeof(key) - AUTH_VECTOR_LEN);

	int outLen = 0;
	const char *currVector = vector;
	while(outLen < passLen)
	{
		memcpy(key + secretlen, currVector, AUTH_VECTOR_LEN);
		CalculateMD5Hash((BYTE *)key, secretlen + AUTH_VECTOR_LEN, (BYTE *)&pwd_out[outLen]);
		currVector = &pwd_out[outLen];

		for(int i = 0; i < AUTH_VECTOR_LEN; i++)
		{
			pwd_out[outLen] ^= passbuf[outLen];
			outLen++;
		}
	}

	return outLen;
}


/*
 * Encrypt/decrypt string attributes, style 1.
 *
 * See RFC 2868, section 3.5 for details. Currently probably indeed 
 * only useful for Tunnel-Password, but why make it a special case, 
 * now we have a generic flags mechanism in place anyway...
 *
 * It's optimized a little for speed, but it could probably be better.
 */

#define CLEAR_STRING_LEN	256 	/* The RFC says it is */
#define SECRET_LEN		32	/* Max. in client.c */
#define MD5_LEN			16	/* The algorithm specifies it */
#define SALT_LEN		2	/* The RFC says it is */

static void encrypt_attr_style_1(char *secret, char *vector, VALUE_PAIR *vp)
{
	char clear_buf[CLEAR_STRING_LEN];
	char work_buf[SECRET_LEN + AUTH_VECTOR_LEN + SALT_LEN];
	char digest[MD5_LEN];
	char *i, *o;
	WORD salt; /* salt in network order */
	int clear_len;
	int work_len;
	int secret_len;
	int n;

	/*
	 * Create the string we'll actually be processing by copying up to 255
	 * bytes of original cleartext, padding it with zeroes to a multiple of
	 * 16 bytes and inserting a length octet in front.
	 */

	/* Limit length */
	clear_len = vp->length;
	if (clear_len > CLEAR_STRING_LEN - 1)
	{
		clear_len = CLEAR_STRING_LEN - 1;
	}

	/* Write the 'original' length byte and copy the buffer */
	*clear_buf = clear_len;
	memcpy(clear_buf + 1, vp->strvalue, clear_len);

	/* From now on, the length byte is included with the byte count */
	clear_len++;

	/* Pad the string to a multiple of 1 chunk */
	if (clear_len % MD5_LEN)
	{
		memset(clear_buf+clear_len, 0, MD5_LEN - (clear_len % MD5_LEN));
	}

	/* Define input and number of chunks to process */
	i = clear_buf;
	clear_len = (clear_len + (MD5_LEN - 1)) / MD5_LEN;	

	/* Define output and starting length */
	o = vp->strvalue;
	vp->length = sizeof(salt);

	/*
	 * Fill in salt. Must be unique per attribute that uses it in the same 
	 * packet, and the most significant bit must be set - see RFC 2868.
	 *
	 * FIXME: this _may_ be too simple. For now we just take the vp 
	 * pointer, which should be different between attributes, xor-ed with 
	 * the first longword of the vector to make it a little more unique.
	 *
	 * Oh, and sizeof(long) always == sizeof(void*) in our part of the
	 * universe, right? (*BSD, Solaris, Linux, DEC Unix...)
	 */
	salt = htons((WORD)((((long)vp ^ *(long *)vector) & 0xffff) | 0x8000));
	memcpy(o, &salt, sizeof(salt));
	o += sizeof(salt);

	/* Create a first working buffer to calc the MD5 hash over */
	secret_len = (int)strlen(secret);	/* already limited by read_clients */
	memcpy(work_buf, secret, secret_len);
	memcpy(work_buf + secret_len, vector, AUTH_VECTOR_LEN);
	memcpy(work_buf + secret_len + AUTH_VECTOR_LEN, &salt, sizeof(salt));
	work_len = secret_len + AUTH_VECTOR_LEN + sizeof(salt);

	for(; clear_len; clear_len--)
	{
		/* Get the digest */
		CalculateMD5Hash((BYTE *)work_buf, work_len, (BYTE *)digest);

		/* Xor the clear text to get the output chunk and next buffer */
		for(n = 0; n < MD5_LEN; n++)
		{
			*(work_buf + secret_len + n) = *o++ = *i++ ^ digest[n];
		}

		/* This is the size of the next working buffer */
		work_len = secret_len + MD5_LEN;

		/* Increment the output length */
		vp->length += MD5_LEN;
	}
}


/*
 * void encrypt_attr(char *secret, char *vector, VALUE_PAIR *vp);
 *
 * Encrypts vp->strvalue using style vp->flags.encrypt, possibly using 
 * a request authenticator passed in vector and the shared secret.
 *
 * This should always succeed.
 */

static void encrypt_attr(char *secret, char *vector, VALUE_PAIR *vp)
{
	switch(vp->flags.encrypt)
	{
		case 0:
			/* Normal, cleartext. */
			break;

		case 1:
			/* Tunnel Password (see RFC 2868, section 3.5). */
			encrypt_attr_style_1(secret, vector, vp);
			break;

		default:
			/* Unknown style - don't send the cleartext! */
			vp->length = 19;
			memcpy(vp->strvalue, "UNKNOWN_ENCR_METHOD", vp->length);
			nxlog_write(MSG_RADIUS_UNKNOWN_ENCR_METHOD, EVENTLOG_ERROR_TYPE,
					"dd", vp->flags.encrypt, vp->attribute);
	}
}


//
// Build radius packet. We assume that the header part
// of AUTH_HDR has already been filled in, we just
// fill auth->data with the A/V pairs from reply.
//

static int rad_build_packet(AUTH_HDR *auth, int auth_len,
		VALUE_PAIR *reply, char *msg, char *secret,
		char *vector, int *send_buffer)
{
	VALUE_PAIR *vp;
	u_short total_length;
	u_char *ptr, *length_ptr;
	char digest[16];
	int vendorpec;
	int len;
	UINT32 lvalue;

	total_length = AUTH_HDR_LEN;

	/*
	 * Load up the configuration values for the user
	 */
	ptr = auth->data;
	for (vp = reply; vp; vp = vp->next)
	{
		/*
		 * Check for overflow.
		 */
		if (total_length + vp->length + 16 >= auth_len)
		{
			break;
		}

		/*
		 * This could be a vendor-specific attribute.
		 */
		length_ptr = NULL;
		if ((vendorpec = VENDOR(vp->attribute)) > 0)
		{
			*ptr++ = PW_VENDOR_SPECIFIC;
			length_ptr = ptr;
			*ptr++ = 6;
			lvalue = htonl(vendorpec);
			memcpy(ptr, &lvalue, 4);
			ptr += 4;
			total_length += 6;
		} 
		else if (vp->attribute > 0xff)
		{
			/*
			 * Ignore attributes > 0xff
			 */
			continue;
		}
		else
		{
			vendorpec = 0;
		}

#ifdef ATTRIB_NMC
		if (vendorpec == VENDORPEC_USR)
		{
			lvalue = htonl(vp->attribute & 0xFFFF);
			memcpy(ptr, &lvalue, 4);
			total_length += 2;
			*length_ptr  += 2;
			ptr          += 4;
		}
		else
#endif
			*ptr++ = (vp->attribute & 0xFF);

		switch(vp->type)
		{

			case PW_TYPE_STRING:
				/*
				 * FIXME: this is just to make sure but
				 * should NOT be needed. In fact I have no
				 * idea if it is needed :)
				 */
				if (vp->length == 0 && vp->strvalue[0] != 0)
				{
					vp->length = (int)strlen(vp->strvalue);
				}
				if (vp->length >= AUTH_STRING_LEN)
				{
					vp->length = AUTH_STRING_LEN - 1;
				}

				/*
				 * If the flags indicate a encrypted attribute, handle 
				 * it here. I don't want to go through the reply list 
				 * another time just for transformations like this.
				 */
				if (vp->flags.encrypt)
				{
					encrypt_attr(secret, vector, vp);
				}

				/*
				 * vp->length is the length of the string value; len
				 * is the length of the string field in the packet.
				 * Normally, these are the same, but if a tag is
				 * inserted only len will reflect this.
				 *
				 * Bug fixed: for tagged attributes with 'tunnel-pwd'
				 * encryption, the tag is *always* inserted, regardless
				 * of its value! (Another strange thing in RFC 2868...)
				 */
				len = vp->length + (vp->flags.has_tag && (TAG_VALID(vp->flags.tag) || vp->flags.encrypt == 1));

#ifdef ATTRIB_NMC
				if (vendorpec != VENDORPEC_USR)
#endif
					*ptr++ = len + 2;
				if (length_ptr)
					*length_ptr += len + 2;

				/* Insert the tag (sorry about the fast ugly test...) */
				if (len > vp->length) *ptr++ = vp->flags.tag;

				/* Use the original length of the string value */
				memcpy(ptr, vp->strvalue, vp->length);
				ptr += vp->length; /* here too */
				total_length += len + 2;
				break;

			case PW_TYPE_INTEGER:
			case PW_TYPE_DATE:
			case PW_TYPE_IPADDR:
				len = sizeof(UINT32) + (vp->flags.has_tag && vp->type != PW_TYPE_INTEGER);
#ifdef ATTRIB_NMC
				if (vendorpec != VENDORPEC_USR)
#endif
					*ptr++ = len + 2;
				if (length_ptr) *length_ptr += len + 2;

				/* Handle tags */
				lvalue = vp->lvalue;
				if (vp->flags.has_tag)
				{
					if (vp->type == PW_TYPE_INTEGER)
					{
						/* Tagged integer: MSB is tag */
						lvalue = (lvalue & 0xffffff) | ((vp->flags.tag & 0xff) << 24);
					}
					else
					{
						/* Something else: insert the tag */
						*ptr++ = vp->flags.tag;
					}
				}
				lvalue = htonl(lvalue);
				memcpy(ptr, &lvalue, sizeof(UINT32));
				ptr += sizeof(UINT32);
				total_length += len + 2;
				break;

			default:
				break;
		}
	}

	/*
	 * Append the user message
	 * FIXME: add multiple PW_REPLY_MESSAGEs if it
	 * doesn't fit into one.
	 */
	if (msg && msg[0])
	{
		len = (int)strlen(msg);
		if (len > 0 && len < AUTH_STRING_LEN-1)
		{
			*ptr++ = PW_REPLY_MESSAGE;
			*ptr++ = len + 2;
			memcpy(ptr, msg, len);
			ptr += len;
			total_length += len + 2;
		}
	}

	auth->length = htons(total_length);

	if (auth->code != PW_AUTHENTICATION_REQUEST && auth->code != PW_STATUS_SERVER)
	{
		/*
		 * Append secret and calculate the response digest
		 */
		len = (int)strlen(secret);
		if (total_length + len < auth_len)
		{
			memcpy((char *)auth + total_length, secret, len);
			CalculateMD5Hash((BYTE *)auth, total_length + len, (BYTE *)digest);
			memcpy(auth->vector, digest, AUTH_VECTOR_LEN);
			memset(send_buffer + total_length, 0, len);
		}
	}

	return total_length;
}


//
// Receive result from server
//

static int result_recv(UINT32 host, WORD udp_port, char *buffer, int length, BYTE *vector, char *secretkey)
{
	AUTH_HDR *auth;
	int totallen, secretlen;
	char reply_digest[AUTH_VECTOR_LEN];
	char calc_digest[AUTH_VECTOR_LEN];
	TCHAR szHostName[32];

	auth = (AUTH_HDR *)buffer;
	totallen = ntohs(auth->length);

	if(totallen != length)
	{
		DbgPrintf(3, _T("RADIUS: Received invalid reply length from server (want %d - got %d)"), totallen, length);
		return 8;
	}

	// Verify the reply digest
	memcpy(reply_digest, auth->vector, AUTH_VECTOR_LEN);
	memcpy(auth->vector, vector, AUTH_VECTOR_LEN);
	secretlen = (int)strlen(secretkey);
	memcpy(buffer + length, secretkey, secretlen);
	CalculateMD5Hash((BYTE *)auth, length + secretlen, (BYTE *)calc_digest);

	if(memcmp(reply_digest, calc_digest, AUTH_VECTOR_LEN) != 0)
	{
		DbgPrintf(3, _T("RADIUS: Received invalid reply digest from server"));
	}

	IpToStr(ntohl(host), szHostName);
	DbgPrintf(3, _T("RADIUS: Packet from host %s code=%d, id=%d, length=%d"),
             szHostName, auth->code, auth->id, totallen);
	return (auth->code == PW_AUTHENTICATION_REJECT) ? 1 : 0;
}


//
// Authenticate user via RADIUS using primary or secondary server
//

static int DoRadiusAuth(const char *cLogin, const char *cPasswd, bool useSecondaryServer, TCHAR *serverName)
{
	AUTH_HDR *auth;
	VALUE_PAIR *req, *vp;
	UINT32 server_ip, local_ip = 0;
	struct sockaddr saremote;
	struct sockaddr_in *sin;
	struct timeval		tv;
	fd_set readfds;
	socklen_t salen;
	int port, result = 0, length, i;
	int nRetries, nTimeout;
	SOCKET sockfd;
	int send_buffer[512];
	int recv_buffer[512];
	BYTE vector[AUTH_VECTOR_LEN];
	char szSecret[256];

	ConfigReadStr(useSecondaryServer ? _T("RADIUSSecondaryServer") : _T("RADIUSServer"), serverName, 256, _T("none"));
	ConfigReadStrA(useSecondaryServer ? _T("RADIUSSecondarySecret"): _T("RADIUSSecret"), szSecret, 256, "netxms");
	port = ConfigReadInt(useSecondaryServer ? _T("RADIUSSecondaryPort") : _T("RADIUSPort"), PW_AUTH_UDP_PORT);
	nRetries = ConfigReadInt(_T("RADIUSNumRetries"), 5);
	nTimeout = ConfigReadInt(_T("RADIUSTimeout"), 3);

	if (!_tcscmp(serverName, _T("none")))
	{
		DbgPrintf(4, _T("RADIUS: %s server set to none, skipping"), useSecondaryServer ? _T("secondary") : _T("primary"));
		return 10;
	}

	// Set up AUTH structure.
	memset(send_buffer, 0, sizeof(send_buffer));
	auth = (AUTH_HDR *)send_buffer;
	auth->code = PW_AUTHENTICATION_REQUEST;
	random_vector(auth->vector);
	auth->id = getpid() & 255;

	// Create attribute chain
	req = NULL;

	// User name
	vp = paircreate(PW_USER_NAME, PW_TYPE_STRING, "User-Name");
	strncpy(vp->strvalue, cLogin, AUTH_STRING_LEN);
	vp->length = min((int)strlen(cLogin), AUTH_STRING_LEN);
	pairadd(&req, vp);

	// Password
	vp = paircreate(PW_PASSWORD, PW_TYPE_STRING, "User-Password");
	vp->length = rad_pwencode(cPasswd, vp->strvalue, szSecret, (char *)auth->vector);
	pairadd(&req, vp);

	// Resolve hostname.
	server_ip = ResolveHostName(serverName);
	if ((server_ip == INADDR_NONE) || (server_ip == INADDR_ANY))
	{
		DbgPrintf(3, _T("RADIUS: cannot resolve server name \"%s\""), serverName);
		pairfree(req);
		return 3;
	}

	// Open a socket.
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == INVALID_SOCKET)
	{
		DbgPrintf(3, _T("RADIUS: Cannot create socket"));
		pairfree(req);
		return 5;
	}

	sin = (struct sockaddr_in *)&saremote;
	memset(sin, 0, sizeof (saremote));
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = server_ip;
	sin->sin_port = htons(port);

	// Build final radius packet.
	length = rad_build_packet(auth, sizeof(send_buffer),
			req, NULL, szSecret, (char *)auth->vector, send_buffer);
	memcpy(vector, auth->vector, sizeof(vector));
	pairfree(req);

	// Send the request we've built.
	for(i = 0; i < nRetries; i++)
	{
		if (i > 0)
		{
			DbgPrintf(3, _T("RADIUS: Re-sending request..."));
		}
		sendto(sockfd, (char *)auth, length, 0, &saremote, sizeof(struct sockaddr_in));

		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);
		tv.tv_sec = nTimeout;
		tv.tv_usec = 0;
		if (select(SELECT_NFDS(sockfd + 1), &readfds, NULL, NULL, &tv) == 0)	
		{
			continue;
		}

		salen = sizeof(saremote);
		result = recvfrom(sockfd, (char *)recv_buffer, sizeof(recv_buffer), 0, &saremote, &salen);
		if (result >= 0)
		{
			break;
		}

		ThreadSleepMs(1000);
	}

	if (result > 0 && i < nRetries)
	{
		result = result_recv(sin->sin_addr.s_addr, sin->sin_port,
				(char *)recv_buffer, result, vector,
				szSecret);
	}
	else
	{
		result = 7;
	}

	closesocket(sockfd);
	return result;
}


//
// Authenticate user via RADIUS
//

bool RadiusAuth(const TCHAR *login, const TCHAR *passwd)
{
	static bool useSecondary = false;

	TCHAR server[256];
#ifdef UNICODE
	char mbLogin[256], mbPasswd[256];
	WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, login, -1, mbLogin, 256, NULL, NULL);
	WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, passwd, -1, mbPasswd, 256, NULL, NULL);
	mbLogin[255] = 0;
	mbPasswd[255] = 0;
	int result = DoRadiusAuth(mbLogin, mbPasswd, useSecondary, server);
#else
	int result = DoRadiusAuth(login, passwd, useSecondary, server);
#endif
	if ((result == 3) || (result == 7) || (result == 10))	// Bad server name, timeout, comm. error, or server not configured
	{
		useSecondary = !useSecondary;
		DbgPrintf(3, _T("RADIUS: unable to use %s server, switching to %s"), useSecondary ? _T("primary") : _T("secondary"), useSecondary ? _T("secondary") : _T("primary"));
#ifdef UNICODE
		result = DoRadiusAuth(mbLogin, mbPasswd, useSecondary, server);
#else
		result = DoRadiusAuth(login, passwd, useSecondary, server);
#endif
	}
	nxlog_write((result == 0) ? MSG_RADIUS_AUTH_SUCCESS : MSG_RADIUS_AUTH_FAILED, NXLOG_INFO, "ss", login, server);
	return result == 0;
}
