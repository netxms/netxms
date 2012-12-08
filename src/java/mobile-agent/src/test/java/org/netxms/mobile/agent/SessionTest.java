/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.mobile.agent;

import junit.framework.TestCase;

/**
 * Base class for NetXMS client library testing.
 * 
 * Please note that all tests expects that NetXMS server is running 
 * on local machine, with user admin and no password.
 * Change appropriate constants if needed.
 */
public class SessionTest extends TestCase
{
	private static final String serverAddress = "127.0.0.1";
	private static final int serverPort = Session.DEFAULT_CONN_PORT;
	private static final String deviceId = "0000000000";
	private static final String loginName = "admin";
	private static final String password = "";

	protected Session connect(boolean useEncryption) throws Exception
	{
		Session session = new Session(serverAddress, serverPort, deviceId, loginName, password, useEncryption);
		session.connect();
		return session;
	}

	protected Session connect() throws Exception
	{
		return connect(false);
	}
}
