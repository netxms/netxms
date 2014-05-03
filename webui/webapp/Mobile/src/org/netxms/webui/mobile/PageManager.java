/**
 * 
 */
package org.netxms.webui.mobile;

import org.netxms.webui.mobile.pages.AbstractPage;

/**
 * Page manager interface
 */
public interface PageManager
{
   /**
    * Open new page on top of view stack
    * 
    * @param page
    */
   public void openPage(AbstractPage page);

   /**
    * Open new page on top of view stack and optionally reset stack.
    * 
    * @param page
    * @param resetStack
    */
   public void openPage(AbstractPage page, boolean resetStack);

   /**
    * Close current page and activate previous one
    */
   public void back();
   
   /**
    * Show navigation panel
    */
   public void showNavigationPanel();
}
