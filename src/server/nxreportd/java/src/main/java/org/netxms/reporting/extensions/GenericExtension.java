/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.reporting.extensions;

import java.io.IOException;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Base class for reporting server extensions
 */
public abstract class GenericExtension
{
   private static final Logger logger = LoggerFactory.getLogger(GenericExtension.class);
   private static final int[] PROTOCOL_COMPONENTS = { ProtocolVersion.INDEX_FULL };

   protected NXCSession session = null;

   /**
    * Connect to NetXMS server using login and password authentication.
    *
    * @param server server host name
    * @param login login name
    * @param password password
    */
   public void connect(String server, String login, String password)
   {
      logger.debug("Connecting to NetXMS server " + server + " as " + login);
      connect(server, null, login, password);
   }

   /**
    * Connect to NetXMS server using login/password or authentication token.
    *
    * @param server server host name
    * @param token authentication token
    */
   public void connect(String server, String token)
   {
      logger.debug("Connecting to NetXMS server " + server + " using authentication token");
      connect(server, token, null, null);
   }

   /**
    * Connect to NetXMS server using login/password or authentication token.
    *
    * @param server server host name
    * @param token authentication token
    * @param login login name
    * @param password password
    */
   private void connect(String server, String token, String login, String password)
   {
      session = new NXCSession(server);
      try
      {
         session.connect(PROTOCOL_COMPONENTS);
         if (token != null)
            session.login(token);
         else
            session.login(login, password);
         session.syncEventTemplates();
         session.syncObjects();
         onConnect(session);
      }
      catch(Exception e)
      {
         logger.error("Cannot connect to NetXMS server", e);
      }
   }

   /**
    * Hook method called after successful connection to NetXMS server. Default implementation does nothing.
    *
    * @param session client session
    * @throws IOException on communication failure
    * @throws NXCException on client failure
    */
   protected void onConnect(NXCSession session) throws IOException, NXCException
   {
   }

   /**
    * Disconnect from server
    */
   public void disconnect()
   {
      if (session != null)
      {
         session.disconnect();
         session = null;
      }
   }
}
