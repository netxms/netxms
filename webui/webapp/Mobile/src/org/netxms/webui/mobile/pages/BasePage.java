/**
 * 
 */
package org.netxms.webui.mobile.pages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.webui.mobile.widgets.NavigationBar;
import com.eclipsesource.tabris.ui.AbstractPage;
import com.eclipsesource.tabris.ui.PageData;
import com.eclipsesource.tabris.widgets.swipe.Swipe;
import com.eclipsesource.tabris.widgets.swipe.SwipeContext;
import com.eclipsesource.tabris.widgets.swipe.SwipeItem;
import com.eclipsesource.tabris.widgets.swipe.SwipeItemProvider;

/**
 * Base page for all client pages
 */
public abstract class BasePage extends AbstractPage
{
   protected NXCSession session;
   
   private Swipe swipe;
   private SwipeItem[] swipeItems = new SwipeItem[2];
   private NavigationBar navigation;
   private Composite content;
   
   /* (non-Javadoc)
    * @see com.eclipsesource.tabris.ui.AbstractPage#createContent(org.eclipse.swt.widgets.Composite, com.eclipsesource.tabris.ui.PageData)
    */
   @Override
   public final void createContent(Composite parent, final PageData pageData)
   {
      session = (NXCSession)ConsoleSharedData.getSession();
      
      parent.setLayout(new FillLayout());
      
      swipeItems[0] = new SwipeItem() {
         @Override
         public Control load(Composite parent)
         {
            navigation = new NavigationBar(parent);
            return navigation;
         }
         
         @Override
         public boolean isPreloadable()
         {
            return true;
         }
         
         @Override
         public void deactivate(SwipeContext context)
         {
         }
         
         @Override
         public void activate(SwipeContext context)
         {
            setTitle("Navigation");
            Point size = navigation.getSize();
            size.x -= 100;
            navigation.setSize(size);
         }
      };
      
      swipeItems[1] = new SwipeItem() {
         @Override
         public Control load(Composite parent)
         {
            content = new Composite(parent, SWT.NONE);
            content.setLayout(new FillLayout());
            createPageContent(content, pageData);
            return content;
         }
         
         @Override
         public boolean isPreloadable()
         {
            return true;
         }
         
         @Override
         public void deactivate(SwipeContext context)
         {
         }
         
         @Override
         public void activate(SwipeContext context)
         {
            setTitle(getTitle());
         }
      };

      /*
      swipe = new Swipe(parent, new SwipeItemProvider() {
         @Override
         public int getItemCount()
         {
            return swipeItems.length;
         }
         
         @Override
         public SwipeItem getItem(int index)
         {
            return swipeItems[index];
         }
      });
      swipe.show(1);
      */
      content = new Composite(parent, SWT.NONE);
      content.setLayout(new FillLayout());
      createPageContent(content, pageData);
      
   }

   /**
    * Create actual page content.
    * 
    * @param parent
    * @param pageData
    */
   protected abstract void createPageContent(Composite parent, PageData pageData);
   
   /**
    * Get page title
    * 
    * @return
    */
   protected abstract String getTitle();
}
