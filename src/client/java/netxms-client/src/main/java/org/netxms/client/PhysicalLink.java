package org.netxms.client;

import org.netxms.base.NXCPMessage;

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
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @param id the id to set
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
    * @param leftSite the leftFront to set
    */
   public void setLeftFront(boolean leftFront)
   {
      this.leftFront = leftFront;
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
    * @param rightFront the rightSFront to set
    */
   public void setRightFront(boolean rightFront)
   {
      this.rightFront = rightFront;
   }
}
