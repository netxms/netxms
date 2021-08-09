package org.netxms.client.businessservices;

import java.util.Date;
import org.netxms.base.NXCPMessage;

public class BusinessServiceTicket
{
   private long id;
   private long serviceId;
   private long checkId;
   private String checkDescription;
   private Date creationTime;
   private Date closeTime;
   private String reason;

   /**
    * @param msg NXCPMessage with data
    * @param base base id
    */
   public BusinessServiceTicket(NXCPMessage msg, long base)
   {
      id = msg.getFieldAsInt64(base);
      serviceId = msg.getFieldAsInt32(base + 1);
      checkId = msg.getFieldAsInt32(base + 2);
      creationTime = msg.getFieldAsDate(base + 3);
      closeTime = msg.getFieldAsDate(base + 4);
      reason = msg.getFieldAsString(base + 5);
      checkDescription = msg.getFieldAsString(base + 6);
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @return the serviceId
    */
   public long getServiceId()
   {
      return serviceId;
   }

   /**
    * @return the checkId
    */
   public long getCheckId()
   {
      return checkId;
   }

   /**
    * @return the checkDescription
    */
   public String getCheckDescription()
   {
      return checkDescription;
   }

   /**
    * @return the creationTime
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * @return the closeTime
    */
   public Date getCloseTime()
   {
      return closeTime;
   }

   /**
    * @return the reason
    */
   public String getReason()
   {
      return reason;
   }
}
