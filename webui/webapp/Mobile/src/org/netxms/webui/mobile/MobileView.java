/**
 * 
 */
package org.netxms.webui.mobile;

import java.util.Stack;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.BrowserNavigation;
import org.eclipse.rap.rwt.client.service.BrowserNavigationEvent;
import org.eclipse.rap.rwt.client.service.BrowserNavigationListener;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StackLayout;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.webui.mobile.pages.AbstractPage;
import org.netxms.webui.mobile.pages.ObjectBrowser;
import org.netxms.webui.mobile.widgets.NavigationBar;
import org.netxms.webui.mobile.widgets.NavigationPanel;

/**
 * @author Victor
 *
 */
public class MobileView extends ViewPart implements PageManager
{
   public static final String ID = "org.netxms.webui.mobile.MobileView";
   
   private NavigationBar navigationBar;
   private Composite viewArea;
   private Stack<AbstractPage> viewStack = new Stack<AbstractPage>();
   private BrowserNavigation navigationService;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      ConsoleSharedData.setProperty("MobileUI.PageManagerInstance", this);
      
      fixWorkbenchLayout(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      layout.horizontalSpacing = 0;
      parent.setLayout(layout);
      
      navigationBar = new NavigationBar(parent);
      navigationBar.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
      
      viewArea = new Composite(parent, SWT.NONE);
      viewArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      viewArea.setLayout(new StackLayout());
      viewArea.setBackground(viewArea.getDisplay().getSystemColor(SWT.COLOR_DARK_GRAY));
      
      navigationService = RWT.getClient().getService(BrowserNavigation.class);
      navigationService.addBrowserNavigationListener(new BrowserNavigationListener() {
         @Override
         public void navigated(BrowserNavigationEvent event)
         {
            back();
         }
      });
      
      openPage(new ObjectBrowser(7));
   }
   
   /**
    * @param parent
    */
   private void fixWorkbenchLayout(Composite parent)
   {
      GridLayout topLayout = new GridLayout();
      topLayout.marginWidth = 0;
      topLayout.marginHeight = 0;
      topLayout.verticalSpacing = 0;
      topLayout.horizontalSpacing = 0;
      
      parent.getParent().getParent().getParent().getParent().setBackground(new Color(parent.getDisplay(), 255, 255, 255));
      parent.getParent().getParent().getParent().getParent().setLayout(topLayout);
      for(Control c : parent.getParent().getParent().getParent().getParent().getChildren())
      {
         System.out.println(c);
         if (c != parent.getParent().getParent().getParent())
         {
            GridData gd = new GridData();
            gd.exclude = true;
            c.setLayoutData(gd);
            c.setVisible(false);
         }
      }
      parent.getParent().getParent().getParent().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      parent.getParent().getParent().getParent().getParent().layout(true, true);
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
   }

   @Override
   public void showNavigationPanel()
   {
      NavigationPanel p = new NavigationPanel(viewArea, SWT.NONE);
      p.setSize(100, 100);
      p.moveAbove(((StackLayout)viewArea.getLayout()).topControl);
   }
      
   /* (non-Javadoc)
    * @see org.netxms.webui.mobile.PageManager#openPage(org.netxms.webui.mobile.pages.AbstractPage)
    */
   @Override
   public void openPage(AbstractPage page)
   {
      openPage(page, false);
   }
   
   /* (non-Javadoc)
    * @see org.netxms.webui.mobile.PageManager#openPage(org.netxms.webui.mobile.pages.AbstractPage, boolean)
    */
   @Override
   public void openPage(AbstractPage page, boolean resetStack)
   {
      if (resetStack)
      {
         for(AbstractPage p : viewStack)
            p.close();
         viewStack.clear();
      }
      viewStack.push(page);
      page.open(this, viewArea);
      ((StackLayout)viewArea.getLayout()).topControl = page.getControl();
      viewArea.layout();
      navigationService.pushState(page.toString(), page.toString());
   }
   
   /* (non-Javadoc)
    * @see org.netxms.webui.mobile.PageManager#back()
    */
   @Override
   public void back()
   {
      if (viewStack.size() == 0)
         return;
      
      AbstractPage page = viewStack.pop();
      if (page != null)
      {
         page.close();
         if (viewStack.size() > 0)
         {
            page = viewStack.peek();
            ((StackLayout)viewArea.getLayout()).topControl = page.getControl();
            viewArea.layout();
         }
      }
   }
}
