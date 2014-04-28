/**
 * 
 */
package org.netxms.webui.mobile;

import java.io.Serializable;
import org.eclipse.rap.rwt.application.Application;
import org.eclipse.rap.rwt.application.Application.OperationMode;
import org.eclipse.rap.rwt.application.ApplicationConfiguration;
import org.eclipse.rap.rwt.application.EntryPoint;
import org.eclipse.rap.rwt.application.EntryPointFactory;

/**
 * Application configuration for mobile client
 */
public class Configuration implements ApplicationConfiguration
{
   /**
    * Custom entry point factory which creates separate UI configuration for each session
    */
   private class MobileClientEntryPointFactory implements EntryPointFactory, Serializable
   {
      @Override
      public EntryPoint create()
      {
         return new MobileClientEntryPoint();
      }
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.rap.rwt.application.ApplicationConfiguration#configure(org.eclipse.rap.rwt.application.Application)
    */
   @Override
   public void configure(Application application)
   {
      application.addEntryPoint("/client", new MobileClientEntryPointFactory(), null);
      application.setOperationMode(OperationMode.SWT_COMPATIBILITY);
   }
}
