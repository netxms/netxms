package org.netxms.client.events;

import org.netxms.base.NXCPMessage;

public class EventObject
{
   public static final int FLAG_WRITE_TO_LOG = 0x0001;
   public static final int FLAG_EVENT_GROUP = 0x0002;
   
   private static final long GROUP_ID_FLAG = 0x80000000L;
   
   protected long code;
   protected String name;
   protected String description;
   
   /**
    * Create either event template or event group from message
    * 
    * @param msg NXCPMessage
    * @param base bas field id
    * @return EventObject
    */
   public static EventObject createFromMessage(NXCPMessage msg, long base)
   {
      if ((msg.getFieldAsInt64(base + 1) & GROUP_ID_FLAG) != 0)
         return new EventGroup(msg, base);
      else
         return new EventTemplate(msg, base);
   }
   
   /**
    * Copy constructor.
    * 
    * @param src Original event object
    */
   protected EventObject(final EventObject src)
   {
      setAll(src);
   }
   
   /**
    * Create new empty event object
    * @param code event object code
    */
   protected EventObject(long code)
   {
      this.code = code;
      name = "";
      description = "";
   }
   
   /**
    * Create event object from NXCP message
    * 
    * @param msg NXCPMessage
    * @param base base field id
    */
   protected EventObject(final NXCPMessage msg, long base)
   {
      code = msg.getFieldAsInt64(base + 1);
      description = msg.getFieldAsString(base + 2);
      name = msg.getFieldAsString(base + 3);
   }   

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
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
    * @return the code
    */
   public long getCode()
   {
      return code;
   }

   /**
    * @param code the code to set
    */
   public void setCode(long code)
   {
      this.code = code;
   }

   /**
    * Set all attributes from another event object.
    * 
    * @param src Original event object
    */
   public void setAll(EventObject src)
   {
      code = src.code;
      name = src.name;
      description = src.description;
   }
}
