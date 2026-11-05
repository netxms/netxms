/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
import java.util.UUID;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.AiMessageStatus;
import org.netxms.client.constants.AiMessageType;

/**
 * AI message
 */
public class AiMessage
{
   private long id;
   private UUID guid;
   private AiMessageType messageType;
   private Date creationTime;
   private Date expirationTime;
   private long originId;
   private String originType;
   private long relatedObjectId;
   private String title;
   private String text;
   private String spawnTaskData;
   private AiMessageStatus status;

   /**
    * Create message object from NXCP message
    *
    * @param msg NXCP message
    * @param baseId base ID for fields
    */
   public AiMessage(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt64(baseId);
      guid = msg.getFieldAsUUID(baseId + 1);
      messageType = AiMessageType.getByValue(msg.getFieldAsInt32(baseId + 2));
      creationTime = msg.getFieldAsDate(baseId + 3);
      expirationTime = msg.getFieldAsDate(baseId + 4);
      originId = msg.getFieldAsInt64(baseId + 5);
      originType = msg.getFieldAsString(baseId + 6);
      relatedObjectId = msg.getFieldAsInt64(baseId + 7);
      title = msg.getFieldAsString(baseId + 8);
      text = msg.getFieldAsString(baseId + 9);
      spawnTaskData = msg.getFieldAsString(baseId + 10);
      status = AiMessageStatus.getByValue(msg.getFieldAsInt32(baseId + 11));
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @return the guid
    */
   public UUID getGuid()
   {
      return guid;
   }

   /**
    * @return the messageType
    */
   public AiMessageType getMessageType()
   {
      return messageType;
   }

   /**
    * @return the creationTime
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * @return the expirationTime
    */
   public Date getExpirationTime()
   {
      return expirationTime;
   }

   /**
    * Get ID of the entity that created this message: AI task ID or AI operator instance ID, or chat ID for messages created
    * from a chat.
    *
    * @return origin ID
    */
   public long getOriginId()
   {
      return originId;
   }

   /**
    * Get type of the entity that created this message: "chat", "task", or "operator" (empty string if not known).
    *
    * @return origin type
    */
   public String getOriginType()
   {
      return originType;
   }

   /**
    * @return the relatedObjectId
    */
   public long getRelatedObjectId()
   {
      return relatedObjectId;
   }

   /**
    * @return the title
    */
   public String getTitle()
   {
      return title;
   }

   /**
    * @return the text
    */
   public String getText()
   {
      return text;
   }

   /**
    * @return the spawnTaskData
    */
   public String getSpawnTaskData()
   {
      return spawnTaskData;
   }

   /**
    * @return the status
    */
   public AiMessageStatus getStatus()
   {
      return status;
   }

   /**
    * Check if message is an approval request
    *
    * @return true if message is an approval request
    */
   public boolean isApprovalRequest()
   {
      return messageType == AiMessageType.APPROVAL_REQUEST;
   }

   /**
    * Check if message is expired
    *
    * @return true if message is expired
    */
   public boolean isExpired()
   {
      return expirationTime.before(new Date());
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "AiMessage [id=" + id + ", guid=" + guid + ", messageType=" + messageType + ", creationTime=" + creationTime + ", expirationTime=" + expirationTime + ", originId=" + originId +
            ", originType=" + originType + ", relatedObjectId=" + relatedObjectId + ", title=" + title + ", text=" + text + ", spawnTaskData=" + spawnTaskData + ", status=" + status + "]";
   }
}
