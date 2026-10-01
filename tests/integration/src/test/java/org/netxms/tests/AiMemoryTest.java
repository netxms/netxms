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
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.List;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ai.AiMemoryEntry;
import org.netxms.client.constants.AiMemoryScope;
import org.netxms.client.constants.RCC;
import org.netxms.utilities.TestHelper;

/**
 * Tests for AI memory store management API
 */
public class AiMemoryTest extends AbstractSessionTest
{
   private static final String TITLE = "Integration test entry";

   /**
    * Find entry by ID in list.
    *
    * @param entries entry list
    * @param id entry ID
    * @return entry or null
    */
   private static AiMemoryEntry findEntry(List<AiMemoryEntry> entries, int id)
   {
      for(AiMemoryEntry e : entries)
         if (e.getId() == id)
            return e;
      return null;
   }

   /**
    * Delete leftover entries with test title in given scope.
    *
    * @param session client session
    * @param scope scope
    * @throws Exception on error
    */
   private static void cleanup(NXCSession session, AiMemoryScope scope) throws Exception
   {
      for(AiMemoryEntry e : session.getAiMemoryEntries())
         if ((e.getScope() == scope) && e.getTitle().equals(TITLE))
            session.deleteAiMemoryEntry(e.getId());
   }

   /**
    * Create, list, modify, lock, and delete entry in environment scope (requires "manage AI memory" right)
    */
   @Test
   public void testEnvironmentEntryLifecycle() throws Exception
   {
      final NXCSession session = connectAndLogin();
      cleanup(session, AiMemoryScope.ENVIRONMENT);

      AiMemoryEntry entry = new AiMemoryEntry(AiMemoryScope.ENVIRONMENT, 0, TITLE, "Created by integration test");
      int id = session.modifyAiMemoryEntry(entry);
      assertTrue(id > 0);

      try
      {
         AiMemoryEntry stored = findEntry(session.getAiMemoryEntries(), id);
         assertNotNull(stored);
         assertEquals(AiMemoryScope.ENVIRONMENT, stored.getScope());
         assertEquals(0, stored.getScopeId());
         assertEquals(TITLE, stored.getTitle());
         assertEquals("Created by integration test", stored.getContent());
         assertFalse(stored.isCreatedByModel());
         assertEquals(AiMemoryEntry.SOURCE_HUMAN, stored.getSourceType());
         assertEquals(session.getUserId(), stored.getSourceUserId());
         assertFalse(stored.isLocked());
         assertNotNull(stored.getCreationTime());

         // Duplicate title within scope is rejected
         AiMemoryEntry duplicate = new AiMemoryEntry(AiMemoryScope.ENVIRONMENT, 0, TITLE.toUpperCase(), "Duplicate");
         NXCException e = assertThrows(NXCException.class, () -> session.modifyAiMemoryEntry(duplicate));
         assertEquals(RCC.NAME_ALEARDY_EXISTS, e.getErrorCode());

         // Modify content and lock
         stored.setContent("Modified by integration test");
         stored.setLocked(true);
         assertEquals(id, session.modifyAiMemoryEntry(stored));

         stored = findEntry(session.getAiMemoryEntries(), id);
         assertNotNull(stored);
         assertEquals("Modified by integration test", stored.getContent());
         assertTrue(stored.isLocked());
      }
      finally
      {
         session.deleteAiMemoryEntry(id);
      }

      assertEquals(null, findEntry(session.getAiMemoryEntries(), id));
      NXCException e = assertThrows(NXCException.class, () -> session.deleteAiMemoryEntry(id));
      assertEquals(RCC.NO_SUCH_RECORD, e.getErrorCode());

      session.disconnect();
   }

   /**
    * Entry in user scope defaults to current user when scope ID is the caller's ID
    */
   @Test
   public void testUserEntry() throws Exception
   {
      final NXCSession session = connectAndLogin();
      cleanup(session, AiMemoryScope.USER);

      AiMemoryEntry entry = new AiMemoryEntry(AiMemoryScope.USER, session.getUserId(), TITLE, "Prefers short answers");
      int id = session.modifyAiMemoryEntry(entry);
      try
      {
         AiMemoryEntry stored = findEntry(session.getAiMemoryEntries(), id);
         assertNotNull(stored);
         assertEquals(AiMemoryScope.USER, stored.getScope());
         assertEquals(session.getUserId(), stored.getScopeId());
      }
      finally
      {
         session.deleteAiMemoryEntry(id);
      }

      // Non-existing user is rejected
      AiMemoryEntry invalid = new AiMemoryEntry(AiMemoryScope.USER, 0x3FFFFFF0, TITLE, "Invalid user");
      NXCException e = assertThrows(NXCException.class, () -> session.modifyAiMemoryEntry(invalid));
      assertEquals(RCC.INVALID_USER_ID, e.getErrorCode());

      session.disconnect();
   }

   /**
    * Entry in object scope is bound to an object and rejected for non-existing object
    */
   @Test
   public void testObjectEntry() throws Exception
   {
      final NXCSession session = connectAndLogin();
      cleanup(session, AiMemoryScope.OBJECT);

      long objectId = TestHelper.findManagementServer(session).getObjectId();
      AiMemoryEntry entry = new AiMemoryEntry(AiMemoryScope.OBJECT, (int)objectId, TITLE, "Note about management server");
      int id = session.modifyAiMemoryEntry(entry);
      try
      {
         AiMemoryEntry stored = findEntry(session.getAiMemoryEntries(), id);
         assertNotNull(stored);
         assertEquals(AiMemoryScope.OBJECT, stored.getScope());
         assertEquals(objectId, stored.getScopeId());
      }
      finally
      {
         session.deleteAiMemoryEntry(id);
      }

      AiMemoryEntry invalid = new AiMemoryEntry(AiMemoryScope.OBJECT, 0x7FFFFFF0, TITLE, "Invalid object");
      NXCException e = assertThrows(NXCException.class, () -> session.modifyAiMemoryEntry(invalid));
      assertEquals(RCC.INVALID_OBJECT_ID, e.getErrorCode());

      session.disconnect();
   }
}
