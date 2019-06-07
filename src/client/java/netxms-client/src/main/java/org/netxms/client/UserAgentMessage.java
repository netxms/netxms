package org.netxms.client;

import java.util.Arrays;
import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * User agent message class
 */
public class UserAgentMessage
{
   private long id;
   private String message;
   private long[] objectList;
   private String objNames;
   private Date startTime;
   private Date endTime;
   private boolean recalled;
   
   /**
    * Constructor for UsearAgentMessage class
    * 
    * @param response response message form server
    * @param base base id of object
    * @param session session
    */
   public UserAgentMessage(NXCPMessage response, long base, NXCSession session)
   {
      id = response.getFieldAsInt32(base);
      message = response.getFieldAsString(base + 1);
      objectList = response.getFieldAsUInt32Array(base + 2);
      
      Arrays.sort(objectList); //Sort objects for comparator in table view
      StringBuilder sb = new StringBuilder();
      for(int i = 0; i < objectList.length; i++)
      {
         sb.append(session.findObjectById(objectList[i]).getObjectName());
         if(i+1 < objectList.length)
         {
            sb.append(", ");
         }
      }
      objNames = sb.toString();
      startTime = response.getFieldAsDate(base + 3);
      endTime = response.getFieldAsDate(base + 4);
      recalled = response.getFieldAsBoolean(base + 5);
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }
   
   /**
    * @return the message
    */
   public String getMessage()
   {
      return message;
   }
   
   /**
    * @return the objectList
    */
   public long[] getObjectList()
   {
      return objectList;
   }
   
   public String getObjectNamesAsString()
   {
      return objNames;
   }
   
   /**
    * @return the startTime
    */
   public Date getStartTime()
   {
      return startTime;
   }
   
   /**
    * @return the endTime
    */
   public Date getEndTime()
   {
      return endTime;
   }
   
   /**
    * @return the recalled
    */
   public boolean isRecalled()
   {
      return recalled;
   }
}
