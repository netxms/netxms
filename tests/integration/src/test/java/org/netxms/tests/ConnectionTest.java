/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.Random;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;

/**
 * Basic connection tests
 */
public class ConnectionTest extends AbstractSessionTest
{	
   @Test
   public void testConnect() throws Exception
   {
      final NXCSession session = connect();

      assertEquals(TestConstants.USER_ID, session.getUserId());
      assertTrue(session.isServerComponentRegistered("CORE"));
      
      System.out.println("Server components:");
      for(String c : session.getRegisteredServerComponents())
         System.out.println("   " + c);
      
      Thread.sleep(2000);
      session.disconnect();
   }

   @Test
   public void testEncryptedConnect() throws Exception
   {
      final NXCSession session = connect(true);

      assertEquals(TestConstants.USER_ID, session.getUserId());
      assertTrue(session.isServerComponentRegistered("CORE"));
      
      Thread.sleep(2000);
      session.disconnect();
   }
   
   @Test
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

   @Test
   public void testMultipleConnections() throws Exception
	{
	   Thread[] t = new Thread[TestConstants.CONNECTION_POOL];
	   for(int i = 0; i < t.length; i++)
	   {
	      t[i] = new Thread(new Runnable() {
            @Override
            public void run()
            {
               try
               {
                  Random rand = new Random();
                  Thread.sleep(rand.nextInt(60000) + 1000);
                  final NXCSession session = connect(true);
                  
                  session.syncObjects();
                  session.syncEventTemplates();
                  session.syncUserDatabase();

                  Thread.sleep(rand.nextInt(60000) + 10000);
                  session.disconnect();
               }
               catch(Exception e)
               {
                  e.printStackTrace();
               }
            }
         });
	      t[i].start();
         System.out.println("Thread #" + (i + 1) + " started");
	   }
	   
      for(int i = 0; i < t.length; i++)
      {
         t[i].join();
         System.out.println("Thread #" + (i + 1) + " stopped");
      }
	}
}
