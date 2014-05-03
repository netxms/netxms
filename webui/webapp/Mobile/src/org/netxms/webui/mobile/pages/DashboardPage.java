/**
 * 
 */
package org.netxms.webui.mobile.pages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.dashboard.widgets.DashboardControl;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * @author Victor
 *
 */
public class DashboardPage extends AbstractPage
{
   private long objectId;
   
   /**
    * 
    */
   public DashboardPage(long objectId)
   {
      this.objectId = objectId;
   }

   /* (non-Javadoc)
    * @see org.netxms.webui.mobile.pages.AbstractPage#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Composite createContent(Composite parent)
   {
      NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      Dashboard dashboard = (Dashboard)session.findObjectById(objectId, Dashboard.class);
      if (dashboard != null)
      {
         DashboardControl dbc = new DashboardControl(parent, SWT.NONE, dashboard, null, false);
         setTitle(dashboard.getObjectName());
         return dbc;
      }
      else
      {
         setTitle("[" + objectId + "]");
         return new Composite(parent, SWT.NONE);
      }
   }
}
