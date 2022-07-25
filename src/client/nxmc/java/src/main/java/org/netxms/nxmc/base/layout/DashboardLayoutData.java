/**
 * 
 */
package org.netxms.nxmc.base.layout;

/**
 * Control's data for dashboard layout
 */
public class DashboardLayoutData
{
   public static final DashboardLayoutData DEFAULT = new DashboardLayoutData();
   
   public int horizontalSpan = 1;
   public int verticalSpan = 1;
   public boolean fill = true;
   public int heightHint = -1;
}
