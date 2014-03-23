/**
 * 
 */
package org.netxms.webui.mobile.pages;

import com.eclipsesource.tabris.ui.PageData;

/**
 * Root for "Dashboards" subtree
 */
public class Dashboards extends ObjectBrowser
{
   /* (non-Javadoc)
    * @see org.netxms.webui.mobile.pages.ObjectBrowser#setRootObject(com.eclipsesource.tabris.ui.PageData)
    */
   @Override
   protected void setRootObject(PageData pageData)
   {
      rootObjectId = 7;
   }
}
