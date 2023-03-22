/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import org.netxms.base.NXCPMessage;

/**
 * Represents physical link between devices or patch panels
 */
public class PhysicalLink
{
   private long id;
   private String description;
   private long leftObjectId;
   private long leftPatchPanelId;
   private int leftPortNumber;
   private boolean leftFront;
   private long rightObjectId;
   private long rightPatchPanelId;
   private int rightPortNumber;
   private boolean rightFront;
   
   /**
    * Constructor for new object
    */
   public PhysicalLink()
   {
      id = 0;
      description = "";
      leftObjectId = 0;
      leftPatchPanelId = 0;
      leftPortNumber = 0;
      leftFront = true;
      rightObjectId = 0;
      rightPatchPanelId = 0;
      rightPortNumber = 0;
      rightFront = true;
   }

   /**
    * Object constructor form message 
    * 
    * @param msg message
    * @param base base id
    */
   public PhysicalLink(NXCPMessage msg, long base)
   {
      id = msg.getFieldAsInt32(base++);
      description = msg.getFieldAsString(base++);
      leftObjectId = msg.getFieldAsInt32(base++);
      leftPatchPanelId = msg.getFieldAsInt32(base++);
      leftPortNumber = msg.getFieldAsInt32(base++);
      leftFront = msg.getFieldAsBoolean(base++);
      rightObjectId = msg.getFieldAsInt32(base++);
      rightPatchPanelId = msg.getFieldAsInt32(base++);
      rightPortNumber = msg.getFieldAsInt32(base++);
      rightFront = msg.getFieldAsBoolean(base++);
   }

   /**
    * Copy constructor for physical link
    * 
    * @param link link to copy form 
    */
   public PhysicalLink(PhysicalLink link)
   {
      id = link.id;
      description = link.description;
      leftObjectId = link.leftObjectId;
      leftPatchPanelId = link.leftPatchPanelId;
      leftPortNumber = link.leftPortNumber;
      leftFront = link.leftFront;
      rightObjectId = link.rightObjectId;
      rightPatchPanelId = link.rightPatchPanelId;
      rightPortNumber = link.rightPortNumber;
      rightFront = link.rightFront;
   }

   /**
    * Fill message with object fields
    * 
    * @param msg message to fill
    * @param base base id 
    */
   public void fillMessage(NXCPMessage msg, long base)
   {
      msg.setFieldInt32(base++, (int)id);
      msg.setField(base++, description);
      msg.setFieldInt32(base++, (int)leftObjectId);
      msg.setFieldInt32(base++, (int)leftPatchPanelId);
      msg.setFieldInt32(base++, leftPortNumber);
      msg.setField(base++, leftFront);
      msg.setFieldInt32(base++, (int)rightObjectId);
      msg.setFieldInt32(base++, (int)rightPatchPanelId);
      msg.setFieldInt32(base++, rightPortNumber);
      msg.setField(base++, rightFront);
   }

   /**
    * Get link object ID
    * 
    * @return link object ID
    */
   public long getId()
   {
      return id;
   }

   /**
    * Set link object ID
    * 
    * @param id new link object ID
    */
   public void setId(long id)
   {
      this.id = id;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @param description the description to set
    */
   public void setDescription(String description)
   {
      this.description = description;
   }

   /**
    * @return the leftObjectId
    */
   public long getLeftObjectId()
   {
      return leftObjectId;
   }

   /**
    * @param leftObjectId the leftObjectId to set
    */
   public void setLeftObjectId(long leftObjectId)
   {
      this.leftObjectId = leftObjectId;
   }

   /**
    * @return the leftPatchPanelId
    */
   public long getLeftPatchPanelId()
   {
      return leftPatchPanelId;
   }

   /**
    * @param leftPatchPanelId the leftPatchPanelId to set
    */
   public void setLeftPatchPanelId(long leftPatchPanelId)
   {
      this.leftPatchPanelId = leftPatchPanelId;
   }

   /**
    * @return the leftPortNumber
    */
   public int getLeftPortNumber()
   {
      return leftPortNumber;
   }

   /**
    * @param leftPortNumber the leftPortNumber to set
    */
   public void setLeftPortNumber(int leftPortNumber)
   {
      this.leftPortNumber = leftPortNumber;
   }

   /**
    * @return the leftFront
    */
   public int getLeftFront()
   {
      return leftFront ? 0 : 1;
   }

   /**
    * Set "isFront" flag for left side of the link
    * 
    * @param front true if this link comes to front side on the left
    */
   public void setLeftFront(boolean front)
   {
      this.leftFront = front;
   }

   /**
    * @return the rightObjectId
    */
   public long getRightObjectId()
   {
      return rightObjectId;
   }

   /**
    * @param rightObjectId the rightObjectId to set
    */
   public void setRightObjectId(long rightObjectId)
   {
      this.rightObjectId = rightObjectId;
   }

   /**
    * @return the rightPatchPanelId
    */
   public long getRightPatchPanelId()
   {
      return rightPatchPanelId;
   }

   /**
    * @param rightPatchPanelId the rightPatchPanelId to set
    */
   public void setRightPatchPanelId(long rightPatchPanelId)
   {
      this.rightPatchPanelId = rightPatchPanelId;
   }

   /**
    * @return the rightPortNumber
    */
   public int getRightPortNumber()
   {
      return rightPortNumber;
   }

   /**
    * @param rightPortNumber the rightPortNumber to set
    */
   public void setRightPortNumber(int rightPortNumber)
   {
      this.rightPortNumber = rightPortNumber;
   }

   /**
    * @return the rightFront
    */
   public int getRightFront()
   {
      return rightFront ? 0 : 1;
   }

   /**
    * Set "isFront" flag for right side of the link
    * 
    * @param front true if this link comes to front side on the right
    */
   public void setRightFront(boolean front)
   {
      this.rightFront = front;
   }
}
