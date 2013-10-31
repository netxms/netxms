/**
 * 
 */
package org.netxms.ui.eclipse.console;

import org.eclipse.jface.action.ContributionItem;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.CoolBar;
import org.eclipse.swt.widgets.CoolItem;
import org.netxms.ui.eclipse.widgets.ServerClock;

/**
 * Contribution item used to show server clock on cool bar
 */
public class ServerClockContributionItem extends ContributionItem
{
   private CoolItem coolItem;
   private Composite clientArea;
   private ServerClock clock;
   
   /**
    * Default constructor
    */
   public ServerClockContributionItem()
   {
      super("org.netxms.ui.eclipse.console.ServerClockContribution");
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.action.ContributionItem#fill(org.eclipse.swt.widgets.CoolBar, int)
    */
   @Override
   public void fill(CoolBar parent, int index)
   {
      coolItem = new CoolItem(parent, SWT.NONE, index);
      coolItem.setData(this);
      
      clientArea = new Composite(parent, SWT.NONE);
      coolItem.setControl(clientArea);
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      clientArea.setLayout(layout);
      
      clock = new ServerClock(clientArea, SWT.NONE);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.grabExcessHorizontalSpace = true;
      clock.setLayoutData(gd);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.action.ContributionItem#dispose()
    */
   @Override
   public void dispose()
   {
      clientArea.dispose();
      coolItem.dispose();
      super.dispose();
   }
}
