/**
 * 
 */
package org.netxms.ui.eclipse.console;

import org.eclipse.jface.action.ContributionItem;
import org.eclipse.rap.rwt.RWT;
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
   public static final String ID = "org.netxms.ui.eclipse.console.ServerClockContribution"; //$NON-NLS-1$
   
   private CoolItem coolItem;
   private Composite clientArea;
   private ServerClock clock;
   
   /**
    * Default constructor
    */
   public ServerClockContributionItem()
   {
      super(ID);
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.action.ContributionItem#fill(org.eclipse.swt.widgets.CoolBar, int)
    */
   @Override
   public void fill(CoolBar parent, int index)
   {
      coolItem = new CoolItem(parent, SWT.NONE, index);
      coolItem.setData(this);
		coolItem.setData(RWT.CUSTOM_VARIANT, "gray"); //$NON-NLS-1$
      
      clientArea = new Composite(parent, SWT.NONE);
      coolItem.setControl(clientArea);
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      clientArea.setLayout(layout);
		clientArea.setData(RWT.CUSTOM_VARIANT, "gray"); //$NON-NLS-1$
      
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
