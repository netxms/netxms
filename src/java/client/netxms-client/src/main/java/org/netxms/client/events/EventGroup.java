package org.netxms.client.events;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.netxms.base.NXCPMessage;

public class EventGroup extends EventObject
{
   private List<Long> eventCodeList;

   /**
    * Create new empty event group
    * 
    * @param code event code
    */
   public EventGroup(long code)
   {
      super(code);
      eventCodeList = new ArrayList<Long>();
   }
   
   /**
    * Create event group from NXCP message
    * 
    * @param msg NXCPMessage
    * @param base base field id
    */
   public EventGroup(NXCPMessage msg, long base)
   {
      super(msg, base);
      eventCodeList = Arrays.asList(msg.getFieldAsUInt32ArrayEx(base + 4));
   }
}
