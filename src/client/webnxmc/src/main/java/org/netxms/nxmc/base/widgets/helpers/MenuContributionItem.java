/**
 * 
 */
package org.netxms.nxmc.base.widgets.helpers;

import org.eclipse.jface.action.IContributionItem;
import org.eclipse.jface.action.IContributionManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.CoolBar;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.ToolBar;

/**
 * Helper class to contribute existing menu as cascade submenu to menu manager
 */
public class MenuContributionItem implements IContributionItem
{
   private String name;
   private Menu menu;
   private boolean visible;

   /**
    * Create new menu contribution item.
    *
    * @param name name of root menu item
    * @param menu menu object to show as cascade menu
    */
   public MenuContributionItem(String name, Menu menu)
   {
      this.name = name;
      this.menu = menu;
      this.visible = true;
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#update(java.lang.String)
    */
   @Override
   public void update(String id)
   {
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#update()
    */
   @Override
   public void update()
   {
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#setVisible(boolean)
    */
   @Override
   public void setVisible(boolean visible)
   {
      this.visible = visible;
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#setParent(org.eclipse.jface.action.IContributionManager)
    */
   @Override
   public void setParent(IContributionManager parent)
   {
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#saveWidgetState()
    */
   @Override
   public void saveWidgetState()
   {
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return visible;
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#isSeparator()
    */
   @Override
   public boolean isSeparator()
   {
      return false;
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#isGroupMarker()
    */
   @Override
   public boolean isGroupMarker()
   {
      return false;
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#isEnabled()
    */
   @Override
   public boolean isEnabled()
   {
      return true;
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#isDynamic()
    */
   @Override
   public boolean isDynamic()
   {
      return false;
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#isDirty()
    */
   @Override
   public boolean isDirty()
   {
      return false;
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#getId()
    */
   @Override
   public String getId()
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#fill(org.eclipse.swt.widgets.CoolBar, int)
    */
   @Override
   public void fill(CoolBar coolBar, int index)
   {
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#fill(org.eclipse.swt.widgets.ToolBar, int)
    */
   @Override
   public void fill(ToolBar toolBar, int index)
   {
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#fill(org.eclipse.swt.widgets.Menu, int)
    */
   @Override
   public void fill(Menu parent, int index)
   {
      MenuItem item = new MenuItem(parent, SWT.CASCADE, index);
      item.setText(name);
      item.setMenu(menu);
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#fill(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void fill(Composite parent)
   {
   }

   /**
    * @see org.eclipse.jface.action.IContributionItem#dispose()
    */
   @Override
   public void dispose()
   {
   }
}
