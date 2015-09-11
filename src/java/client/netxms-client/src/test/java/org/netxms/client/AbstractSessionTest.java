/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
 * <p/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p/>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client;

import junit.framework.TestCase;
import org.netxms.base.Logger;
import org.netxms.base.LoggingFacility;

/**
 * Base class for NetXMS client library testing.
 *
 * Please note that all tests expects that NetXMS server is running 
 * on local machine, with user admin and no password.
 * Change appropriate constants if needed.
 */
public abstract class AbstractSessionTest extends TestCase
{
    protected static final String serverAddress = "127.0.0.1";
    protected static final int serverPort = NXCSession.DEFAULT_CONN_PORT;
    protected static final String loginName = "admin";
    protected static final String password = "";

    protected NXCSession connect(boolean useEncryption) throws Exception
    {
        Logger.setLoggingFacility(new LoggingFacility() {
            @Override
            public void writeLog(int level, String tag, String message, Throwable t)
            {
                System.out.println("[" + tag + "] " + message);
            }
        });

        NXCSession session = new NXCSession(serverAddress, serverPort, useEncryption);
        session.connect(new int[]{ProtocolVersion.INDEX_FULL});
        session.login(loginName, password);
        return session;
    }

    protected NXCSession connect() throws Exception
    {
        return connect(false);
    }
}
