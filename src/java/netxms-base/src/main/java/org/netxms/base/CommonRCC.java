/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.base;

/**
 * This class represents request common completion codes (RCC) that
 * can be sent by any NetXMS-based server.
 */
public class CommonRCC
{
	public static final int SUCCESS = 0;
	public static final int COMPONENT_LOCKED = 1;
	public static final int ACCESS_DENIED = 2;
	public static final int INVALID_REQUEST = 3;
	public static final int TIMEOUT = 4;
	public static final int OUT_OF_STATE_REQUEST = 5;
	public static final int DB_FAILURE = 6;
	public static final int ALREADY_EXIST = 8;
	public static final int COMM_FAILURE = 9;
	public static final int SYSTEM_FAILURE = 10;
	public static final int INVALID_USER_ID = 11;
	public static final int INVALID_ARGUMENT = 12;
	public static final int OUT_OF_MEMORY = 15;
	public static final int IO_ERROR = 16;
	public static final int INCOMPATIBLE_OPERATION = 17;
	public static final int OPERATION_IN_PROGRESS = 23;
	public static final int NOT_IMPLEMENTED = 28;
	public static final int VERSION_MISMATCH = 31;
	public static final int BAD_PROTOCOL = 40;
	public static final int NO_CIPHERS = 42;
	public static final int INVALID_PUBLIC_KEY = 43;
	public static final int INVALID_SESSION_KEY = 44;
	public static final int NO_ENCRYPTION_SUPPORT = 45;
	public static final int INTERNAL_ERROR = 46;
	public static final int TRANSFER_IN_PROGRESS = 54;
	public static final int LOCAL_CRYPTO_ERROR = 71;
	public static final int UNSUPPORTED_AUTH_TYPE = 72;
	public static final int BAD_CERTIFICATE = 73;
	public static final int INVALID_CERT_ID = 74;
	public static final int WEAK_PASSWORD = 87;
	public static final int REUSED_PASSWORD = 88;
	public static final int ENCRYPTION_ERROR = 98;
}
