/**
 * 
 */
package org.netxms.webui.mobile.pages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.netxms.webui.mobile.PageManager;

/**
 * Abstract page class
 */
public abstract class AbstractPage
{
   private PageManager pageManager;
   private Composite page;
   private Composite content;
   
   /**
    * @param parent
    */
   public final void open(PageManager pageManager, Composite parent)
   {
      this.pageManager = pageManager;
      page = new Composite(parent, SWT.NONE);
      page.setLayout(new FillLayout());
      content = createContent(page);
   }
   
   /**
    * Close page
    */
   public final void close()
   {
      dispose();
      page.dispose();
   }
   
   /**
    * @return
    */
   public final Control getControl()
   {
      return page;
   }
   
   public void setTitle(String title)
   {   
   }
   
   /**
    * @return
    */
   protected Composite getContent()
   {
      return content;
   }
   
   /**
    * @return
    */
   protected PageManager getPageManager()
   {
      return pageManager;
   }
   
   /**
    * @return
    */
   protected Display getDisplay()
   {
      return page.getDisplay();
   }
   
   /**
    * @param parent
    * @return
    */
   protected abstract Composite createContent(Composite parent);
   
   /**
    * Page dispose handler
    */
   protected void dispose()
   {
   }
}
