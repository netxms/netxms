package org.netxms.client;

import org.netxms.base.NXCPMessage;

/**
 * Driver configuration template class
 */
public class NCConfigurationTemplate
{
   public boolean needSubject;
   public boolean needRecipient;
   
   /**
    * Constructor 
    * 
    * @param msg message
    * @param base base id
    */
   public NCConfigurationTemplate(final NXCPMessage msg, long base)
   {
      needSubject = msg.getFieldAsBoolean(base);
      needRecipient = msg.getFieldAsBoolean(base+1);
   }
}
