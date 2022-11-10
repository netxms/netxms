/**
 * 
 */
package org.netxms.ui.eclipse.datacollection;

import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.api.ConsoleTweaklet;

/**
 * 
 */
public class DataCollectionTweaklet implements ConsoleTweaklet
{
   /**
    * @see org.netxms.ui.eclipse.console.api.ConsoleTweaklet#postLogin(org.netxms.client.NXCSession)
    */
   @Override
   public void postLogin(NXCSession session)
   {
   }

   /**
    * @see org.netxms.ui.eclipse.console.api.ConsoleTweaklet#preWindowOpen(org.eclipse.ui.application.IWorkbenchWindowConfigurer)
    */
   @Override
   public void preWindowOpen(IWorkbenchWindowConfigurer configurer)
   {
   }

   /**
    * @see org.netxms.ui.eclipse.console.api.ConsoleTweaklet#postWindowCreate(org.eclipse.ui.application.IWorkbenchWindowConfigurer)
    */
   @Override
   public void postWindowCreate(IWorkbenchWindowConfigurer configurer)
   {
      SourceProvider.getInstance().update();
   }
}
