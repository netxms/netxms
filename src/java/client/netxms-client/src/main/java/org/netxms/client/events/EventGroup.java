package org.netxms.client.events;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

public class EventGroup extends EventObject
{
   private List<Long> eventCodeList;

   /**
    * Create new empty event group
    * 
    * @param code event code
    */
   public EventGroup()
   {
      super(0);
      eventCodeList = new ArrayList<Long>();
   }
   
   /**
    * Copy constructor
    * 
    * @param src Original event group object
    */
   public EventGroup(EventGroup src)
   {
      super(src);
      setAll(src);
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
      eventCodeList = new ArrayList<Long>(Arrays.asList(msg.getFieldAsUInt32ArrayEx(base + 4)));
   }
   
   /* (non-Javadoc)
    * @see org.netxms.client.events.EventObject#fillMessage(org.netxms.base.NXCPMessage)
    */
   public void fillMessage(NXCPMessage msg)
   {
      super.fillMessage(msg);
      msg.setField(NXCPCodes.VID_IS_GROUP, true);
      msg.setField(NXCPCodes.VID_EVENT_LIST, eventCodeList.toArray(new Long[eventCodeList.size()]));
   }
   
   /**
    * Check if there are any children in the group
    * 
    * @return true if there are children in the group
    */
   public boolean hasChildren()
   {
      return eventCodeList.size() > 0 ? true : false;
   }
   
   /**
    * Get the array of event codes in the group
    * 
    * @return array of event codes in the group
    */
   public Long[] getEventCodes()
   {
      return eventCodeList.toArray(new Long[eventCodeList.size()]);
   }
   
   /**
    * Check if event is a child of the group
    * 
    * @return true if event is a child
    */
   public boolean hasChild(Long code)
   {
      return eventCodeList.contains(code);
   }
   
   /** 
    * Add child to group
    * 
    * @param code of child
    */
   public void addChild(Long code)
   {
      if (this.code != code)
         eventCodeList.add(code);
   }
   
   /**
    * Remove child from group
    * 
    * @param code of child
    */
   public void removeChild(Long code)
   {
      eventCodeList.remove(code);
   }
   
   /**
    * Set all attributes from another event group.
    * 
    * @param src Original event group
    */
   public void setAll(EventObject src)
   {
      if (!(src instanceof EventGroup))
         return;
      
      super.setAll(src);
      eventCodeList = ((EventGroup)src).eventCodeList;
   }
}
