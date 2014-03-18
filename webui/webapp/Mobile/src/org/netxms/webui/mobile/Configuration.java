/**
 * 
 */
package org.netxms.webui.mobile;

import org.eclipse.rap.rwt.application.Application;
import org.eclipse.rap.rwt.application.Application.OperationMode;
import org.eclipse.rap.rwt.application.ApplicationConfiguration;
import org.netxms.webui.mobile.pages.Alarms;
import org.netxms.webui.mobile.pages.DashboardPage;
import org.netxms.webui.mobile.pages.LoginPage;
import org.netxms.webui.mobile.pages.ObjectBrowser;
import org.netxms.webui.mobile.pages.PredefinedGraphTree;
import com.eclipsesource.tabris.TabrisClientInstaller;
import com.eclipsesource.tabris.ui.PageConfiguration;
import com.eclipsesource.tabris.ui.TabrisUIEntrypointFactory;
import com.eclipsesource.tabris.ui.UIConfiguration;

/**
 * Application configuration for mobile client
 */
public class Configuration implements ApplicationConfiguration
{
   /* (non-Javadoc)
    * @see org.eclipse.rap.rwt.application.ApplicationConfiguration#configure(org.eclipse.rap.rwt.application.Application)
    */
   @Override
   public void configure(Application application)
   {
      TabrisClientInstaller.install(application);
      application.addEntryPoint("/client", new TabrisUIEntrypointFactory(createUIConfiguration()), null);
      application.setOperationMode(OperationMode.SWT_COMPATIBILITY);
   }

   /**
    * @return
    */
   private UIConfiguration createUIConfiguration()
   {
      UIConfiguration uiConfiguration = new UIConfiguration();

      PageConfiguration page = new PageConfiguration("page.alarms", Alarms.class);
      page.setTitle("Alarms");
      page.setTopLevel(true);
      uiConfiguration.addPageConfiguration(page);
      
      page = new PageConfiguration("page.objectBrowser", ObjectBrowser.class);
      page.setTitle("Object Browser");
      page.setTopLevel(true);
      uiConfiguration.addPageConfiguration(page);
      
      page = new PageConfiguration("page.graphs", PredefinedGraphTree.class);
      page.setTitle("Graphs");
      page.setTopLevel(true);
      uiConfiguration.addPageConfiguration(page);
      
      page = new PageConfiguration("page.dashboard", DashboardPage.class);
      page.setTitle("Dashboard");
      uiConfiguration.addPageConfiguration(page);

      page = new PageConfiguration("page.login", LoginPage.class);
      page.setTitle("Dashboard");
      uiConfiguration.addPageConfiguration(page);
      
      return uiConfiguration;
   }
}
