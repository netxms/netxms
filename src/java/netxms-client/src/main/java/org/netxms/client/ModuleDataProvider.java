/**
 * 
 */
package org.netxms.client;

import org.netxms.api.client.services.ServiceHandler;
import org.netxms.base.NXCPMessage;

/**
 * Module data provider
 */
public interface ModuleDataProvider extends ServiceHandler
{
   /**
    * Create module data from NXCP message
    * 
    * @param msg
    * @param baseId
    * @return
    */
   public Object createModuleData(NXCPMessage msg, long baseId);
}
