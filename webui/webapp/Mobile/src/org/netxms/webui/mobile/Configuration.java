/**
 * 
 */
package org.netxms.webui.mobile;

import org.eclipse.rap.rwt.application.Application;
import org.eclipse.rap.rwt.application.Application.OperationMode;
import org.eclipse.rap.rwt.application.ApplicationConfiguration;
import org.netxms.webui.mobile.pages.LoginPage;
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

      PageConfiguration page = new PageConfiguration("page.login", LoginPage.class);
      page.setTopLevel(true);
      page.setTitle("NetXMS Client");
      uiConfiguration.addPageConfiguration(page);
      
      return uiConfiguration;
   }
}
