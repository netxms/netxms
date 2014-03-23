/**
 * 
 */
package org.netxms.webui.mobile.pages;

import java.lang.reflect.InvocationTargetException;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.jobs.LoginJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.webui.mobile.widgets.ProgressWidget;
import com.eclipsesource.tabris.ui.AbstractPage;
import com.eclipsesource.tabris.ui.PageConfiguration;
import com.eclipsesource.tabris.ui.PageData;
import com.eclipsesource.tabris.ui.UIConfiguration;

/**
 * Login page
 */
public class LoginPage extends AbstractPage
{
   private Composite parent;
   private LabeledText login;
   private LabeledText password;
   
   /* (non-Javadoc)
    * @see com.eclipsesource.tabris.ui.AbstractPage#createContent(org.eclipse.swt.widgets.Composite, com.eclipsesource.tabris.ui.PageData)
    */
   @Override
   public void createContent(Composite parent, PageData data)
   {
      this.parent = parent;
      
      NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      if (session != null)
      {
         changeTopLevelPages(session);
      }
      else
      {
         createLoginForm();
      }
   }
   
   /**
    * Create login form
    */
   private void createLoginForm()
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
   
   /**
    * Dispose page content
    */
   private void disposeContent()
   {
      for(Control c : parent.getChildren())
         c.dispose();
   }

   /**
    * "OK" button handler
    */
   private void okPressed()
   {      
      LoginJob job = new LoginJob(Display.getCurrent(), "127.0.0.1", login.getText(), false, false);
      job.setPassword(password.getText());
      
      disposeContent();
      ProgressWidget pw = new ProgressWidget(parent, SWT.NONE);
      pw.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, true));
      parent.layout(true, true);
      
      try
      {
         pw.run(job);
         disposeContent();
         NXCSession session = (NXCSession)ConsoleSharedData.getSession();
         changeTopLevelPages(session);
      }
      catch(InvocationTargetException e)
      {
         e.printStackTrace();
         disposeContent();
         createLoginForm();
      }
   }
   
   /**
    * @param session
    */
   private void changeTopLevelPages(NXCSession session)
   {
      UIConfiguration uiConfiguration = getUIConfiguration();
      
      getUI().getConfiguration();
      
      PageConfiguration page = new PageConfiguration("page.alarms", Alarms.class);
      page.setTitle("Alarms");
      page.setTopLevel(true);
      uiConfiguration.addPageConfiguration(page);
      
      page = new PageConfiguration("page.infrastructureServices", InfrastructureServices.class);
      page.setTitle("Infrastructure Services");
      page.setTopLevel(true);
      uiConfiguration.addPageConfiguration(page);
      
      page = new PageConfiguration("page.dashboards", Dashboards.class);
      page.setTitle("Dashboards");
      page.setTopLevel(true);
      uiConfiguration.addPageConfiguration(page);
      
      page = new PageConfiguration("page.graphs", PredefinedGraphTree.class);
      page.setTitle("Graphs");
      page.setTopLevel(true);
      uiConfiguration.addPageConfiguration(page);
      
      openPage("page.alarms");
      
      uiConfiguration.removePageConfiguration("page.login");      
   }
}
