/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;
import java.util.List;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.constants.RCC;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.AuthenticationToken;
import org.netxms.client.users.User;
import org.netxms.utilities.TestHelper;

/**
 * Tests for single-use authentication tokens.
 */
public class SingleUseTokenTest extends AbstractSessionTest
{
   private static final int TOKEN_VALIDITY_TIME = 600;
   private static final String HANDOFF_USER_NAME = "single-use-handoff-test";
   private static final String HANDOFF_USER_PASSWORD = "Handoff.Test.1";

   /**
    * Login to server with given authentication token. Caller is responsible for disconnecting returned session.
    *
    * @param token token value
    * @return logged in session
    */
   private NXCSession loginWithToken(String token) throws Exception
   {
      NXCSession session = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, true);
      session.connect(new int[] { ProtocolVersion.INDEX_FULL });
      try
      {
         session.login(token);
      }
      catch(Exception e)
      {
         session.disconnect();
         throw e;
      }
      return session;
   }

   /**
    * Login to server with given credentials. Caller is responsible for disconnecting returned session.
    *
    * @param login login name
    * @param password password
    * @return logged in session
    */
   private NXCSession loginWithPassword(String login, String password) throws Exception
   {
      NXCSession session = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, true);
      session.connect(new int[] { ProtocolVersion.INDEX_FULL });
      try
      {
         session.login(login, password);
      }
      catch(Exception e)
      {
         session.disconnect();
         throw e;
      }
      return session;
   }

   /**
    * Wait for session to be disconnected by the server.
    *
    * @param session session to watch
    * @return true if session was disconnected within the timeout
    */
   private boolean waitForDisconnect(NXCSession session) throws Exception
   {
      for(int i = 0; (i < 50) && session.isConnected(); i++)
         Thread.sleep(100);
      return !session.isConnected();
   }

   /**
    * Create (or reuse) a test user with CLOSE_OTHER_SESSIONS flag set - the flag single-use login has to ignore.
    *
    * @param admin session with MANAGE_USERS access right
    * @return test user
    */
   private User prepareHandoffUser(NXCSession admin) throws Exception
   {
      User user = TestHelper.findOrCreateUser(admin, HANDOFF_USER_NAME, HANDOFF_USER_PASSWORD);
      user.setFlags((user.getFlags() | AbstractUserObject.CLOSE_OTHER_SESSIONS)
            & ~(AbstractUserObject.DISABLED | AbstractUserObject.CHANGE_PASSWORD | AbstractUserObject.INTRUDER_LOCKOUT));
      admin.modifyUserDBObject(user, AbstractUserObject.MODIFY_FLAGS);
      admin.setUserPassword(user.getId(), HANDOFF_USER_PASSWORD, null);
      return user;
   }

   @Test
   public void testLoginWithSingleUseToken() throws Exception
   {
      NXCSession issuer = connectAndLogin();
      AuthenticationToken token = issuer.requestAuthenticationToken(false, TOKEN_VALIDITY_TIME, "single-use token test", 0, true);
      assertNotNull(token.getValue());
      assertFalse(token.getValue().isEmpty());
      assertTrue(token.isSingleUse());
      assertFalse(token.isPersistent());

      NXCSession session = loginWithToken(token.getValue());
      try
      {
         assertEquals(issuer.getUserId(), session.getUserId());
      }
      finally
      {
         session.disconnect();
      }
   }

   @Test
   public void testSingleUseTokenCannotBeReused() throws Exception
   {
      NXCSession issuer = connectAndLogin();
      AuthenticationToken token = issuer.requestAuthenticationToken(false, TOKEN_VALIDITY_TIME, "single-use token test", 0, true);

      NXCSession session = loginWithToken(token.getValue());
      session.disconnect();

      try
      {
         loginWithToken(token.getValue()).disconnect();
         fail("Second login with single-use token succeeded");
      }
      catch(NXCException e)
      {
         assertEquals(RCC.ACCESS_DENIED, e.getErrorCode());
      }
   }

   @Test
   public void testOrdinaryTokenIsNotConsumed() throws Exception
   {
      NXCSession issuer = connectAndLogin();
      AuthenticationToken token = issuer.requestAuthenticationToken(false, TOKEN_VALIDITY_TIME, "reusable token test", 0);
      assertFalse(token.isSingleUse());

      for(int i = 0; i < 3; i++)
      {
         NXCSession session = loginWithToken(token.getValue());
         assertEquals(issuer.getUserId(), session.getUserId());
         session.disconnect();
      }
   }

   /**
    * Handing a session over must not disconnect the issuer, even for a user whose sessions are normally
    * closed by a new login. This is the primary use case - a launcher spawning the console must survive it.
    */
   @Test
   public void testSingleUseLoginKeepsOtherSessions() throws Exception
   {
      NXCSession admin = connectAndLogin();
      User user = prepareHandoffUser(admin);

      NXCSession issuer = loginWithPassword(HANDOFF_USER_NAME, HANDOFF_USER_PASSWORD);
      try
      {
         AuthenticationToken token = issuer.requestAuthenticationToken(false, TOKEN_VALIDITY_TIME, "handoff test", 0, true);
         NXCSession session = loginWithToken(token.getValue());
         try
         {
            // Round trip request, because isConnected() alone does not prove the server kept the session
            assertNotNull(issuer.getAlarms());
            assertTrue(issuer.isConnected());
            assertEquals(user.getId(), session.getUserId());
         }
         finally
         {
            session.disconnect();
         }

         // Control check - an ordinary login for the same user must close the issuer session,
         // otherwise the assertion above would hold even without the single-use exception
         NXCSession replacement = loginWithPassword(HANDOFF_USER_NAME, HANDOFF_USER_PASSWORD);
         try
         {
            assertTrue(waitForDisconnect(issuer), "CLOSE_OTHER_SESSIONS is not in effect for test user");
         }
         finally
         {
            replacement.disconnect();
         }
      }
      finally
      {
         if (issuer.isConnected())
            issuer.disconnect();
      }
   }

   /**
    * Every token in a listing must be decoded from its own set of fields
    */
   @Test
   public void testTokenListingReportsEachTokenSeparately() throws Exception
   {
      NXCSession issuer = connectAndLogin();
      AuthenticationToken singleUseToken = issuer.requestAuthenticationToken(false, TOKEN_VALIDITY_TIME, "listing test (single-use)", 0, true);
      AuthenticationToken ephemeralToken = issuer.requestAuthenticationToken(false, TOKEN_VALIDITY_TIME, "listing test (ephemeral)", 0);

      List<AuthenticationToken> tokens = issuer.getAuthenticationTokens(issuer.getUserId());
      AuthenticationToken listedSingleUse = null;
      AuthenticationToken listedEphemeral = null;
      for(AuthenticationToken t : tokens)
      {
         if (singleUseToken.getValue().equals(t.getValue()))
            listedSingleUse = t;
         else if (ephemeralToken.getValue().equals(t.getValue()))
            listedEphemeral = t;
      }

      assertNotNull(listedSingleUse, "Single-use token is missing from listing");
      assertNotNull(listedEphemeral, "Ephemeral token is missing from listing");
      assertTrue(listedSingleUse.isSingleUse());
      assertFalse(listedSingleUse.isPersistent());
      assertFalse(listedEphemeral.isSingleUse());
      assertFalse(listedEphemeral.isPersistent());
      assertEquals("listing test (single-use)", listedSingleUse.getDescription());
      assertEquals("listing test (ephemeral)", listedEphemeral.getDescription());
   }

   @Test
   public void testPersistentSingleUseTokenRejected() throws Exception
   {
      NXCSession issuer = connectAndLogin();
      try
      {
         issuer.requestAuthenticationToken(true, TOKEN_VALIDITY_TIME, "invalid combination test", 0, true);
         fail("Request for persistent single-use token was accepted");
      }
      catch(NXCException e)
      {
         assertEquals(RCC.INVALID_ARGUMENT, e.getErrorCode());
      }
   }
}
