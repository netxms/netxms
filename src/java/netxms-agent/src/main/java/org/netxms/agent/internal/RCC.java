/**
 * NetXMS - open source network management system
 * Copyright (C) 2012 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.agent.internal;

/**
 * Request completion codes for agent
 */
public class RCC
{
	public static final int SUCCESS               = 0;
	public static final int UNKNOWN_COMMAND       = 400;
	public static final int AUTH_REQUIRED         = 401;
	public static final int ACCESS_DENIED         = 403;
	public static final int UNKNOWN_PARAMETER     = 404;
	public static final int REQUEST_TIMEOUT       = 408;
	public static final int AUTH_FAILED           = 440;
	public static final int ALREADY_AUTHENTICATED = 441;
	public static final int AUTH_NOT_REQUIRED     = 442;
	public static final int INTERNAL_ERROR        = 500;
	public static final int NOT_IMPLEMENTED       = 501;
	public static final int OUT_OF_RESOURCES      = 503;
	public static final int NOT_CONNECTED         = 900;
	public static final int CONNECTION_BROKEN     = 901;
	public static final int BAD_RESPONSE          = 902;
	public static final int IO_FAILURE            = 903;
	public static final int RESOURCE_BUSY         = 904;
	public static final int EXEC_FAILED           = 905;
	public static final int ENCRYPTION_REQUIRED   = 906;
	public static final int NO_CIPHERS            = 907;
	public static final int INVALID_PUBLIC_KEY    = 908;
	public static final int INVALID_SESSION_KEY   = 909;
	public static final int CONNECT_FAILED        = 910;
	public static final int MALFORMED_COMMAND     = 911;
	public static final int SOCKET_ERROR          = 912;
	public static final int BAD_ARGUMENTS         = 913;
	public static final int SUBAGENT_LOAD_FAILED  = 914;
	public static final int FILE_OPEN_ERROR       = 915;
	public static final int FILE_STAT_FAILED      = 916;
	public static final int MEM_ALLOC_FAILED      = 917;
	public static final int FILE_DELETE_FAILED    = 918;
}
