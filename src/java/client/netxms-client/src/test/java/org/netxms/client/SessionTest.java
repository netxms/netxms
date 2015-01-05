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
package org.netxms.client;

import org.netxms.base.Logger;
import org.netxms.base.LoggingFacility;
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
	private static final int serverPort = NXCSession.DEFAULT_CONN_PORT;
	private static final String loginName = "admin";
	private static final String password = "";

	protected NXCSession connect(boolean useEncryption) throws Exception
	{
      Logger.setLoggingFacility(new LoggingFacility() {       
         @Override
         public void writeLog(int level, String tag, String message, Throwable t)
         {
            System.out.println("[" + tag + "] " + message);
         }
      });
      
		NXCSession session = new NXCSession(serverAddress, serverPort, loginName, password, useEncryption);
		session.connect();
		return session;
	}

	protected NXCSession connect() throws Exception
	{
		return connect(false);
	}
	
   public void testConnect() throws Exception
   {
      final NXCSession session = connect();

      assertEquals(0, session.getUserId());
      
      Thread.sleep(2000);
      session.disconnect();
   }

   public void testEncryptedConnect() throws Exception
   {
      final NXCSession session = connect(true);

      assertEquals(0, session.getUserId());
      
      Thread.sleep(2000);
      session.disconnect();
   }
   
   public void testIllegalStates() throws Exception
   {
      NXCSession session = connect();
      try
      {
         session.connect();
         assertTrue(false);
      }
      catch(IllegalStateException e)
      {
         System.out.println("IllegalStateException thrown (" + e.getMessage() + ")");
      }
      
      session.disconnect();
      try
      {
         session.connect();
         assertTrue(false);
      }
      catch(IllegalStateException e)
      {
         System.out.println("IllegalStateException thrown (" + e.getMessage() + ")");
      }
   }
}
