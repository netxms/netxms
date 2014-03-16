/**
 * 
 */
package org.netxms.webui.mobile.pages;

import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import com.eclipsesource.tabris.ui.AbstractPage;
import com.eclipsesource.tabris.ui.PageData;

/**
 * Base page for all client pages (non top level)
 */
public abstract class BasePage extends AbstractPage
{
   protected NXCSession session;
   
   /* (non-Javadoc)
    * @see com.eclipsesource.tabris.ui.AbstractPage#createContent(org.eclipse.swt.widgets.Composite, com.eclipsesource.tabris.ui.PageData)
    */
   @Override
   public final void createContent(Composite parent, final PageData pageData)
   {
      session = (NXCSession)ConsoleSharedData.getSession();
      parent.setLayout(new FillLayout());
      createPageContent(parent, pageData);
   }

   /**
    * Create actual page content.
    * 
    * @param parent
    * @param pageData
    */
   protected abstract void createPageContent(Composite parent, PageData pageData);
}
