package org.netxms.client.objecttools;

import org.netxms.base.NXCPMessage;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractNode;

/**
 * Class to hold information about selected node
 */
public class ObjectContextBase
{
   public AbstractNode object;
   public Alarm alarm;
   
   public ObjectContextBase(AbstractNode object, Alarm alarm)
   {
      this.object = object;
      this.alarm = alarm;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      final int prime = 31;
      int result = 1;
      result = prime * result + ((alarm == null) ? 0 : alarm.hashCode());
      result = prime * result + ((object == null) ? 0 : object.hashCode());
      return result;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object obj)
   {
      if (this == obj)
         return true;
      if (obj == null)
         return false;
      if (getClass() != obj.getClass())
         return false;
      ObjectContextBase other = (ObjectContextBase)obj;
      if ((other.object == null) || (this.object == null))
         return (other.object == null) && (this.object == null);
      return other.object.getObjectId() == this.object.getObjectId();
   }

   public void fillMessage(NXCPMessage msg, long varId, String textToExpand)
   {
      msg.setField(varId++, textToExpand);
      msg.setFieldInt32(varId++, object != null ? (int)object.getObjectId() : 0);
      msg.setFieldInt32(varId++, alarm != null ? (int)alarm.getId() : 0);     
   }
}