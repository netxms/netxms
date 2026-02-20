/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.tests;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Base class for NetXMS client library testing.
 *
 * Please note that all tests expects that NetXMS server is running on local machine, with user admin and no password. Change
 * appropriate constants if needed.
 */
public abstract class AbstractSessionTest
{
   private static final Logger logger = LoggerFactory.getLogger(AbstractSessionTest.class);
   private static final String METRIC_NAME = "Server.DB.Queries.Failed";

   private NXCSession session = null;
   private long managementServerId = 0;
   private long initialFailedQueries = -1;

   protected NXCSession connect() throws Exception
   {
      if (session != null)
         session.disconnect();
      session = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, true);
      session.setRecvBufferSize(65536, 33554432);
      session.connect(new int[] { ProtocolVersion.INDEX_FULL });
      return session;
   }

   protected NXCSession connectAndLogin() throws Exception
   {
      NXCSession s = connect();
      s.login(TestConstants.SERVER_LOGIN, TestConstants.SERVER_PASSWORD);
      if (s.isPasswordExpired())
      {
         s.setUserPassword(session.getUserId(), TestConstants.SERVER_PASSWORD, TestConstants.SERVER_PASSWORD);
         s.setServerVariable("DataCollection.DefaultDCIRetentionTime", "10");
      }
      return s;
   }

   /**
    * Get the failed query count by querying internal metric directly
    */
   private long getFailedQueryCount() throws Exception
   {
      if (managementServerId == 0)
         return -1;

      String value = session.queryMetric(managementServerId, DataOrigin.INTERNAL, METRIC_NAME);
      return Long.parseLong(value);
   }

   @BeforeEach
   public void setupSqlErrorCheck()
   {
      initialFailedQueries = -1;
      try
      {
         session = connectAndLogin();

         Node managementServer = TestHelper.findManagementServer(session);
         if (managementServer != null)
         {
            managementServerId = managementServer.getObjectId();
            initialFailedQueries = getFailedQueryCount();
            System.out.println("SQL error check: initial failed queries = " + initialFailedQueries);
         }
         else
         {
            logger.warn("Management server not found - SQL error check disabled for this test");
         }
      }
      catch(Exception e)
      {
         logger.warn("Could not setup SQL error check", e);
      }
   }

   @AfterEach
   public void cleanup()
   {
      // SQL error check
      if (session != null && session.isConnected())
      {
         try
         {
            if (initialFailedQueries >= 0 && managementServerId != 0)
            {
               long currentFailedQueries = getFailedQueryCount();
               System.out.println("SQL error check: final failed queries = " + currentFailedQueries);
               if (currentFailedQueries > initialFailedQueries)
               {
                  long errorCount = currentFailedQueries - initialFailedQueries;
                  throw new AssertionError("SQL errors detected during test: " + errorCount +
                     " failed queries (was " + initialFailedQueries + ", now " + currentFailedQueries + ")");
               }
            }
         }
         catch(AssertionError e)
         {
            throw e;  // Re-throw assertion errors
         }
         catch(Exception e)
         {
            logger.warn("Could not complete SQL error check", e);
         }
      }

      if (session != null)
      {
         try
         {
            session.disconnect();
            session = null;
         }
         catch(Exception e)
         {
            logger.warn("Exception when closing session", e);
         }
      }
   }
}
