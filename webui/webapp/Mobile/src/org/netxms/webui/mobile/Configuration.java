/**
 * 
 */
package org.netxms.webui.mobile;

import org.eclipse.rap.rwt.application.Application;
import org.eclipse.rap.rwt.application.Application.OperationMode;
import org.eclipse.rap.rwt.application.ApplicationConfiguration;
import org.netxms.webui.mobile.pages.Alarms;
import org.netxms.webui.mobile.pages.DashboardPage;
import org.netxms.webui.mobile.pages.HomePage;
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
      
      /*
      ActionConfiguration actionNavigation = new ActionConfiguration("action.navigation", ShowNavigationBar.class);
      actionNavigation.setTitle("Navigation");
      uiConfiguration.addActionConfiguration(actionNavigation);
      */
      
      // Add a top level page
      PageConfiguration homePage = new PageConfiguration("page.home", HomePage.class);
      homePage.setTopLevel(true);
      homePage.setTitle("NetXMS Client");
      uiConfiguration.addPageConfiguration(homePage);

      PageConfiguration page = new PageConfiguration("page.alarms", Alarms.class);
      page.setTopLevel(true);
      page.setTitle("Alarms");
      uiConfiguration.addPageConfiguration(page);
      
      page = new PageConfiguration("page.objectBrowser", ObjectBrowser.class);
      page.setTitle("Object Browser");
      uiConfiguration.addPageConfiguration(page);
      
      page = new PageConfiguration("page.graphs", PredefinedGraphTree.class);
      page.setTopLevel(true);
      page.setTitle("Graphs");
      uiConfiguration.addPageConfiguration(page);
      
      page = new PageConfiguration("page.dashboard", DashboardPage.class);
      page.setTitle("Dashboard");
      uiConfiguration.addPageConfiguration(page);
      
      /*
      uiConfiguration.addTransitionListener(new TransitionListener() {
         @Override
         public void before(UI ui, Page from, Page to)
         {
         }
         
         @Override
         public void after(UI ui, Page from, Page to)
         {
            if ((to instanceof HomePage) && !(from instanceof LoginPage))
            {
               ((AbstractPage)to).openPage("page.login");
            }
         }
      });
      */
      
      return uiConfiguration;
   }
}
