package org.netxms.webui.mobile.pages;

import java.lang.reflect.InvocationTargetException;
import java.sql.ClientInfoStatus;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.jobs.LoginJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ImageCache;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.webui.mobile.Activator;
import org.netxms.webui.mobile.dialogs.ProgressDialog;
import org.netxms.webui.mobile.widgets.ProgressWidget;
import com.eclipsesource.tabris.device.ClientDevice;
import com.eclipsesource.tabris.device.ClientDeviceListener;
import com.eclipsesource.tabris.device.ClientDevice.ConnectionType;
import com.eclipsesource.tabris.device.ClientDevice.Orientation;
import com.eclipsesource.tabris.ui.AbstractPage;
import com.eclipsesource.tabris.ui.PageData;
import com.eclipsesource.tabris.widgets.enhancement.Widgets;

/**
 * Client home page
 */
public class HomePage extends AbstractPage
{
   private Composite content;
   private LabeledText login;
   private LabeledText password;
   
   /* (non-Javadoc)
    * @see com.eclipsesource.tabris.ui.AbstractPage#createContent(org.eclipse.swt.widgets.Composite, com.eclipsesource.tabris.ui.PageData)
    */
   @Override
   public void createContent(final Composite parent, PageData data)
   {
      NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      content = parent;
      if (session != null)
      {
         createHomeScreen(parent);
      }
      else
      {
         createLoginForm(parent);
      }
      
   }
   
   private void createHomeScreen(final Composite parent)
   {
      ClientDevice device = RWT.getClient().getService(ClientDevice.class);
      
      parent.setLayout(new GridLayout(device != null && device.getOrientation() == ClientDevice.Orientation.LANDSCAPE ? 3 : 2, true));
      //parent.setBackground(parent.getDisplay().getSystemColor(SWT.COLOR_DARK_GRAY));
      
      ImageCache imageCache = new ImageCache(parent);

      addSelector(parent, "Alarms", imageCache.add(Activator.getImageDescriptor("icons/alarms.png")), "page.alarms", null);
      PageData pd = new PageData();
      pd.set("rootObject", Long.valueOf(7));
      addSelector(parent, "Dashboards", imageCache.add(Activator.getImageDescriptor("icons/dashboard.png")), "page.objectBrowser", pd);
      pd = new PageData();
      pd.set("rootObject", Long.valueOf(2));
      addSelector(parent, "Nodes", imageCache.add(Activator.getImageDescriptor("icons/nodes.png")), "page.objectBrowser", pd);
      pd = new PageData();
      pd.set("rootObject", Long.valueOf(1));
      addSelector(parent, "Network", imageCache.add(Activator.getImageDescriptor("icons/entire_network.png")), "page.objectBrowser", pd);
      addSelector(parent, "Graphs", imageCache.add(Activator.getImageDescriptor("icons/graphs.png")), "page.graphs", null);
      
      if (device != null)
      {
         device.addClientDeviceListener(new ClientDeviceListener() {
            @Override
            public void orientationChange(Orientation o)
            {
               ((GridLayout)parent.getLayout()).numColumns = (o == ClientDevice.Orientation.LANDSCAPE) ? 3 : 2;
               parent.layout(true, true);
            }
            
            @Override
            public void connectionTypeChanged(ConnectionType ct)
            {
            }
         });
      }
   }

   private void addSelector(Composite parent, String name, Image icon, final String pageId, final PageData pageData)
   {
      Composite selector = new Composite(parent, SWT.BORDER);
      selector.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      
      GridLayout layout = new GridLayout();
      selector.setLayout(layout);
      
      Label image = new Label(selector, SWT.NONE);
      image.setImage(icon);
      image.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, false));
      
      Label text = new Label(selector, SWT.NONE);
      text.setText(name);
      text.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, false));
      
      Widgets.onComposite(selector).addGroupedListener(SWT.TOUCHSTATE_UP, new Listener() {
         @Override
         public void handleEvent(Event event)
         {
            if (pageData != null)
               openPage(pageId, pageData);
            else
               openPage(pageId);
         }
      });
   }
   
   private void createLoginForm(final Composite parent)
   {
      GridLayout layout = new GridLayout();
      parent.setLayout(layout);
      
      login = new LabeledText(parent, SWT.NONE);
      login.setLabel("Login");
      login.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      login.setText("admin");

      password = new LabeledText(parent, SWT.NONE, SWT.BORDER | SWT.PASSWORD);
      password.setLabel("Password");
      password.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      
      Button okButton = new Button(parent, SWT.PUSH);
      okButton.setText("Login");
      okButton.setLayoutData(new GridData(SWT.RIGHT, SWT.CENTER, true, false));
      okButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            okPressed();
         }
      });
   }
   
   private void disposeContent()
   {
      for(Control c : content.getChildren())
         c.dispose();
   }

   private void okPressed()
   {
      LoginJob job = new LoginJob(Display.getCurrent(), "127.0.0.1", login.getText(), false, false);
      job.setPassword(password.getText());
      
      disposeContent();
      ProgressWidget pw = new ProgressWidget(content, SWT.NONE);
      pw.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, true));
      content.layout(true, true);
      
      try
      {
         pw.run(job);
         disposeContent();
         createHomeScreen(content);
         content.layout(true, true);
      }
      catch(InvocationTargetException e)
      {
         e.printStackTrace();
         disposeContent();
         createLoginForm(content);
      }
   }
}
