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
package org.netxms.client.ai;

import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.AiMemoryScope;

/**
 * AI memory entry: fact about the monitored environment, note about a user, or note about an object that persists
 * across AI assistant chats, AI tasks, and AI operator iterations.
 */
public class AiMemoryEntry
{
   /**
    * Source type of entries written by the model from an interactive chat, chat bot session, or one-shot request
    */
   public static final char SOURCE_CHAT = 'C';

   /**
    * Source type of entries written by the model during AI task execution
    */
   public static final char SOURCE_TASK = 'T';

   /**
    * Source type of entries written by the model during AI operator iteration
    */
   public static final char SOURCE_OPERATOR = 'O';

   /**
    * Source type of entries written by a user through the client or web API
    */
   public static final char SOURCE_HUMAN = 'H';

   private int id;
   private AiMemoryScope scope;
   private int scopeId;
   private String title;
   private String content;
   private boolean createdByModel;
   private int sourceUserId;
   private char sourceType;
   private int sourceId;
   private Date creationTime;
   private Date modificationTime;
   private boolean locked;

   /**
    * Create new memory entry definition (for subsequent creation on server).
    *
    * @param scope entry scope
    * @param scopeId scope ID (user ID for user scope, object ID for object scope, 0 for environment scope)
    * @param title entry title (unique within scope)
    * @param content entry content
    */
   public AiMemoryEntry(AiMemoryScope scope, int scopeId, String title, String content)
   {
      id = 0;
      this.scope = scope;
      this.scopeId = scopeId;
      this.title = title;
      this.content = content;
      createdByModel = false;
      sourceUserId = 0;
      sourceType = SOURCE_HUMAN;
      sourceId = 0;
      creationTime = null;
      modificationTime = null;
      locked = false;
   }

   /**
    * Create copy of memory entry (for editing without changing the original).
    *
    * @param src source entry
    */
   public AiMemoryEntry(AiMemoryEntry src)
   {
      id = src.id;
      scope = src.scope;
      scopeId = src.scopeId;
      title = src.title;
      content = src.content;
      createdByModel = src.createdByModel;
      sourceUserId = src.sourceUserId;
      sourceType = src.sourceType;
      sourceId = src.sourceId;
      creationTime = src.creationTime;
      modificationTime = src.modificationTime;
      locked = src.locked;
   }

   /**
    * Create memory entry object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base ID for fields
    */
   public AiMemoryEntry(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      scope = AiMemoryScope.getByValue(msg.getFieldAsInt32(baseId + 1));
      scopeId = msg.getFieldAsInt32(baseId + 2);
      title = msg.getFieldAsString(baseId + 3);
      content = msg.getFieldAsString(baseId + 4);
      createdByModel = msg.getFieldAsBoolean(baseId + 5);
      sourceUserId = msg.getFieldAsInt32(baseId + 6);
      sourceType = (char)msg.getFieldAsInt32(baseId + 7);
      sourceId = msg.getFieldAsInt32(baseId + 8);
      creationTime = msg.getFieldAsDate(baseId + 9);
      modificationTime = msg.getFieldAsDate(baseId + 10);
      locked = msg.getFieldAsBoolean(baseId + 11);
   }

   /**
    * Fill NXCP message with entry data for create/modify request.
    *
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldUInt32(NXCPCodes.VID_RECORD_ID, id);
      msg.setFieldInt16(NXCPCodes.VID_MEMORY_SCOPE, scope.getValue());
      msg.setFieldUInt32(NXCPCodes.VID_SCOPE_ID, scopeId);
      msg.setField(NXCPCodes.VID_TITLE, title);
      msg.setField(NXCPCodes.VID_MESSAGE, content);
      msg.setField(NXCPCodes.VID_LOCKED, locked);
   }

   /**
    * @return entry ID (0 for entries not yet created on server)
    */
   public int getId()
   {
      return id;
   }

   /**
    * @return entry scope
    */
   public AiMemoryScope getScope()
   {
      return scope;
   }

   /**
    * @return scope ID (user ID for user scope, object ID for object scope, 0 for environment scope)
    */
   public int getScopeId()
   {
      return scopeId;
   }

   /**
    * @return entry title
    */
   public String getTitle()
   {
      return title;
   }

   /**
    * @param title new entry title
    */
   public void setTitle(String title)
   {
      this.title = title;
   }

   /**
    * @return entry content
    */
   public String getContent()
   {
      return content;
   }

   /**
    * @param content new entry content
    */
   public void setContent(String content)
   {
      this.content = content;
   }

   /**
    * @return true if entry was created by the model, false if created by a user
    */
   public boolean isCreatedByModel()
   {
      return createdByModel;
   }

   /**
    * @return ID of the user on whose behalf the entry was created
    */
   public int getSourceUserId()
   {
      return sourceUserId;
   }

   /**
    * @return source type (one of SOURCE_* constants)
    */
   public char getSourceType()
   {
      return sourceType;
   }

   /**
    * @return source ID (chat ID, AI task ID, or AI operator instance ID depending on source type; 0 for human source)
    */
   public int getSourceId()
   {
      return sourceId;
   }

   /**
    * @return creation time
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * @return last modification time
    */
   public Date getModificationTime()
   {
      return modificationTime;
   }

   /**
    * @return true if entry is locked (cannot be changed or deleted by the model)
    */
   public boolean isLocked()
   {
      return locked;
   }

   /**
    * @param locked true to lock entry
    */
   public void setLocked(boolean locked)
   {
      this.locked = locked;
   }
}
