/**
 * 
 */
package org.netxms.webui.mobile.pages;

import com.eclipsesource.tabris.ui.PageData;

/**
 * Root for "Infrastructure services" subtree
 */
public class InfrastructureServices extends ObjectBrowser
{
   /* (non-Javadoc)
    * @see org.netxms.webui.mobile.pages.ObjectBrowser#setRootObject(com.eclipsesource.tabris.ui.PageData)
    */
   @Override
   protected void setRootObject(PageData pageData)
   {
      rootObjectId = 2;
   }
}
