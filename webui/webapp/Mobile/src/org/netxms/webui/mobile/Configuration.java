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
import org.netxms.webui.mobile.pages.DashboardPage;
import org.netxms.webui.mobile.pages.LoginPage;
import org.netxms.webui.mobile.pages.ObjectBrowser;
import com.eclipsesource.tabris.TabrisClientInstaller;
import com.eclipsesource.tabris.ui.PageConfiguration;
import com.eclipsesource.tabris.ui.TabrisUIEntryPoint;
import com.eclipsesource.tabris.ui.UIConfiguration;

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
         return new TabrisUIEntryPoint(createUIConfiguration());
      }
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.rap.rwt.application.ApplicationConfiguration#configure(org.eclipse.rap.rwt.application.Application)
    */
   @Override
   public void configure(Application application)
   {
      TabrisClientInstaller.install(application);
      application.addEntryPoint("/client", new MobileClientEntryPointFactory(), null);
      application.setOperationMode(OperationMode.SWT_COMPATIBILITY);
   }

   /**
    * @return
    */
   private UIConfiguration createUIConfiguration()
   {
      UIConfiguration uiConfiguration = new UIConfiguration();

      PageConfiguration page = new PageConfiguration("page.login", LoginPage.class);
      page.setTitle("Login");
      page.setTopLevel(true);
      uiConfiguration.addPageConfiguration(page);
      
      page = new PageConfiguration("page.objectBrowser", ObjectBrowser.class);
      page.setTitle("Object Browser");
      uiConfiguration.addPageConfiguration(page);
      
      page = new PageConfiguration("page.dashboard", DashboardPage.class);
      page.setTitle("Dashboard");
      uiConfiguration.addPageConfiguration(page);

      return uiConfiguration;
   }
}
