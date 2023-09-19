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
import org.netxms.reporting.ServerException;
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
    * @throws ServerException on failure
    */
   public void connect(String server, String login, String password) throws ServerException
   {
      logger.debug("Connecting to NetXMS server " + server + " as " + login);
      connect(server, null, login, password);
   }

   /**
    * Connect to NetXMS server using login/password or authentication token.
    *
    * @param server server host name
    * @param token authentication token
    * @throws ServerException on failure
    */
   public void connect(String server, String token) throws ServerException
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
    * @throws ServerException on failure
    */
   private void connect(String server, String token, String login, String password) throws ServerException
   {
      session = new NXCSession(server);
      try
      {
         session.connect(PROTOCOL_COMPONENTS);
         if (token != null)
            session.login(token);
         else
            session.login(login, password);
         session.syncObjects();
         onConnect(session);
      }
      catch(Exception e)
      {
         try
         {
            session.disconnect();
         }
         catch(Throwable t)
         {
            logger.debug("Unexpected error during connect failure cleanup", t);
         }
         throw new ServerException("Cannot connect to NetXMS server", e);
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
