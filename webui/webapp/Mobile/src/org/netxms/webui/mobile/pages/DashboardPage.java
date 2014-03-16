/**
 * 
 */
package org.netxms.webui.mobile.pages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.dashboard.widgets.DashboardControl;
import com.eclipsesource.tabris.ui.PageData;

/**
 * Dashboard page
 */
public class DashboardPage extends BasePage
{
   private Dashboard dashboard;
   private DashboardControl dbc;
   
   /* (non-Javadoc)
    * @see org.netxms.webui.mobile.pages.BasePage#createPageContent(org.eclipse.swt.widgets.Composite, com.eclipsesource.tabris.ui.PageData)
    */
   @Override
   protected void createPageContent(Composite parent, PageData pageData)
   {
      Long id = pageData.get("object", Long.class);
      dashboard = (Dashboard)((id != null) ? session.findObjectById(id, Dashboard.class) : null);
      if (dashboard != null)
      {
         dbc = new DashboardControl(parent, SWT.NONE, dashboard, null, false);
         setTitle(dashboard.getObjectName());
      }
      else
      {
         setTitle("[" + id + "]");
      }
   }
}
