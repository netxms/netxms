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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.concurrent.CountDownLatch;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.TwoFactorAuthenticationCallback;
import org.netxms.client.constants.AuthenticationType;
import org.netxms.client.constants.RCC;
import org.netxms.client.users.TwoFactorAuthenticationMethod;
import org.netxms.client.users.User;
import org.netxms.utilities.TestHelper;

/**
 * Basic connection tests
 */
public class ConnectionTest extends AbstractSessionTest
{
   @Test
   public void testConnect() throws Exception
   {
      final NXCSession session = connectAndLogin();

      assertEquals(TestConstants.USER_ID, session.getUserId());
      assertTrue(session.isServerComponentRegistered("CORE"));
      
      System.out.println("Server components:");
      for(String c : session.getRegisteredServerComponents())
         System.out.println("   " + c);
      
      Thread.sleep(2000);
      session.disconnect();
   }

   @Test
   public void testIllegalStates() throws Exception
   {
      NXCSession session = connectAndLogin();
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
         t[i] = new Thread(() -> {
            try
            {
               Random rand = new Random();
               Thread.sleep(rand.nextInt(60000) + 1000);
               final NXCSession session = connectAndLogin();

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

   private static class Worker implements Runnable
   {
      CountDownLatch latch;
      NXCSession session;
      Exception exception = null;
      boolean success = false;
      boolean test2FA;

      Worker(CountDownLatch latch, NXCSession session, boolean test2FA)
      {
         this.latch = latch;
         this.session = session;
         this.test2FA = test2FA;
      }

      @Override
      public void run()
      {
         try
         {
            latch.countDown();
            latch.await();
            if (test2FA)
            {
               session.login(AuthenticationType.PASSWORD, TestConstants.SERVER_LOGIN_2FA, TestConstants.SERVER_PASSWORD_2FA, null, null, new TwoFactorAuthenticationCallback() {
                  @Override
                  public int selectMethod(List<String> methods)
                  {
                     return 0;
                  }

                  @Override
                  public void saveTrustedDeviceToken(long serverId, String username, byte[] token)
                  {
                  }

                  @Override
                  public String getUserResponse(String challenge, String qrLabel, boolean trustedDevicesAllowed)
                  {
                     return "123456";
                  }

                  @Override
                  public byte[] getTrustedDeviceToken(long serverId, String username)
                  {
                     return null;
                  }
               });
            }
            else
            {
               session.login(TestConstants.SERVER_LOGIN, TestConstants.SERVER_PASSWORD);
            }
            success = true;
         }
         catch(Exception e)
         {
            if (!(e instanceof NXCException) || ((((NXCException)e).getErrorCode() != RCC.OUT_OF_STATE_REQUEST) && (((NXCException)e).getErrorCode() != RCC.FAILED_2FA_VALIDATION)))
               exception = e;
         }
      }
   }

   @Test
   public void testMultipleLogins() throws Exception
   {
      final NXCSession session = connect();

      Thread[] t = new Thread[16];
      Worker[] w = new Worker[t.length];
      CountDownLatch latch = new CountDownLatch(t.length);
      for(int i = 0; i < t.length; i++)
      {
         w[i] = new Worker(latch, session, false);
         t[i] = new Thread(w[i]);
         t[i].start();
      }

      // Exactly one worker should be able to login
      int count = 0;
      for(int i = 0; i < t.length; i++)
      {
         t[i].join();
         if (w[i].exception != null)
            throw new Exception("Error reported by worker thread", w[i].exception);
         else if (w[i].success)
            count++;
      }
      assertEquals(1, count);

      session.disconnect();
   }

   private void prepare2FATests() throws Exception
   {
      NXCSession session = connectAndLogin();
      try
      {
         List<TwoFactorAuthenticationMethod> methods = session.get2FAMethods();
         if (methods.isEmpty())
         {
            TwoFactorAuthenticationMethod m = new TwoFactorAuthenticationMethod("EMAIL", "Test Email Method", "Message", "ChannelName=SMTP-Text\n");
            session.modify2FAMethod(m);
            methods.add(m);
         }

         User user = TestHelper.findOrCreateUser(session, TestConstants.SERVER_LOGIN_2FA, TestConstants.SERVER_PASSWORD_2FA);
         if (user.getTwoFactorAuthMethodBindings().isEmpty())
         {
            Map<String, Map<String, String>> bindings = new HashMap<>();
            Map<String, String> config = new HashMap<>();
            config.put("Recipient", "noreply@netxms.org");
            config.put("Subject", "Access code");
            bindings.put(methods.get(0).getName(), config);
            user.setTwoFactorAuthMethodBindings(bindings);
            session.modifyUserDBObject(user);
         }
      }
      finally
      {
         session.disconnect();
      }
   }

   @Test
   public void testMultipleFailed2FALogins() throws Exception
   {
      prepare2FATests();

      final NXCSession session = connect();

      Thread[] t = new Thread[16];
      Worker[] w = new Worker[t.length];
      CountDownLatch latch = new CountDownLatch(t.length);
      for(int i = 0; i < t.length; i++)
      {
         w[i] = new Worker(latch, session, true);
         t[i] = new Thread(w[i]);
         t[i].start();
      }

      // No workers should be able to login, but no exceptions should be reported
      int count = 0;
      for(int i = 0; i < t.length; i++)
      {
         t[i].join();
         if (w[i].exception != null)
            throw new Exception("Error reported by worker thread", w[i].exception);
         else if (w[i].success)
            count++;
      }
      assertEquals(0, count);

      session.disconnect();
   }
}
