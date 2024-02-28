/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.console;

import org.eclipse.core.commands.common.NotDefinedException;
import org.eclipse.jface.bindings.BindingManager;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.ExitConfirmation;
import org.eclipse.rap.rwt.client.service.JavaScriptExecutor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.eclipse.ui.internal.WorkbenchWindow;
import org.eclipse.ui.internal.keys.BindingService;
import org.eclipse.ui.keys.IBindingService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.LicenseProblem;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Workbench window advisor
 */
@SuppressWarnings("restriction")
public class ApplicationWorkbenchWindowAdvisor extends WorkbenchWindowAdvisor
{
   /**
    * @param configurer
    */
   public ApplicationWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer)
   {
      super(configurer);
   }

   /**
    * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#createActionBarAdvisor(org.eclipse.ui.application.IActionBarConfigurer)
    */
   @Override
   public ActionBarAdvisor createActionBarAdvisor(IActionBarConfigurer configurer)
   {
      return new ApplicationActionBarAdvisor(configurer);
   }

   /**
    * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#preWindowOpen()
    */
   @Override
   public void preWindowOpen()
   {
      RegionalSettings.updateFromPreferences();
      
      IWorkbenchWindowConfigurer configurer = getWindowConfigurer();
      configurer.setShowPerspectiveBar(!ConsoleSharedData.getSession().getClientConfigurationHintAsBoolean("PerspectiveSwitcher.Enable", true));
      configurer.setShowStatusLine(false);
      configurer.setTitle(Messages.get().ApplicationWorkbenchWindowAdvisor_AppTitle);
      configurer.setShellStyle(SWT.NO_TRIM);
   }

   /**
    * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#postWindowCreate()
    */
   @Override
   public void postWindowCreate()
   {
      super.postWindowCreate();

      NXCSession session = ConsoleSharedData.getSession();
		
      // Changes the page title at runtime
      JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
      if (executor != null)
      {
         StringBuilder js = new StringBuilder();
         js.append("document.title = "); //$NON-NLS-1$
         js.append("\"");
         js.append(BrandingManager.getInstance().getProductName());
         js.append(" - [");
         js.append(session.getUserName());
         js.append("@");
         js.append(session.getServerAddress());
         js.append("]");
         js.append("\"");
         executor.execute(js.toString());
      }

		BindingService service = (BindingService)getWindowConfigurer().getWindow().getWorkbench().getService(IBindingService.class);
		BindingManager bindingManager = service.getBindingManager();
		try
		{
			bindingManager.setActiveScheme(service.getScheme("org.netxms.ui.eclipse.defaultKeyBinding")); //$NON-NLS-1$
		}
		catch(NotDefinedException e)
		{
			e.printStackTrace();
		}

		final Shell shell = getWindowConfigurer().getWindow().getShell();
		shell.setMaximized(true);

		Menu menuBar = shell.getMenuBar();
		if (menuBar != null)
		   menuBar.setData(RWT.CUSTOM_VARIANT, "menuBar"); //$NON-NLS-1$
   }

   /**
    * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#postWindowOpen()
    */
   @Override
   public void postWindowOpen()
   {
      ((WorkbenchWindow)getWindowConfigurer().getWindow()).getCoolBarManager2().setLockLayout(true);

      String dashboardId = Application.getParameter("dashboard"); //$NON-NLS-1$
      if (dashboardId != null)
      {
         showDashboard(dashboardId, false);
      }
      else
      {
         dashboardId = Application.getParameter("fullscreen-dashboard"); //$NON-NLS-1$
         if (dashboardId != null)
         {
            showDashboard(dashboardId, true);
         }
      }

      showMessageOfTheDay();
   }

   /**
    * Show dashboard
    * 
    * @param dashboardId
    */
   private void showDashboard(String dashboardId, boolean fullScreen)
   {
      NXCSession session = ConsoleSharedData.getSession();

      long objectId;
      try
      {
         objectId = Long.parseLong(dashboardId);
      }
      catch(NumberFormatException e)
      {
         AbstractObject object = session.findObjectByName(dashboardId, (AbstractObject o) -> o instanceof Dashboard);
         if ((object == null) || !(object instanceof Dashboard))
         {
            MessageDialogHelper.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Error, String.format(Messages.get().ApplicationWorkbenchWindowAdvisor_CannotOpenDashboard, dashboardId));
            return;
         }
         objectId = object.getObjectId();
      }

      Dashboard dashboard = session.findObjectById(objectId, Dashboard.class);
      if (dashboard == null)
      {
         MessageDialogHelper.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Error, String.format(Messages.get().ApplicationWorkbenchWindowAdvisor_CannotOpenDashboard, dashboardId));
         return;
      }

      IWorkbenchPage page = getWindowConfigurer().getWindow().getActivePage();
      try
      {
         IViewPart view = page.showView("org.netxms.ui.eclipse.dashboard.views.DashboardView", Long.toString(objectId), IWorkbenchPage.VIEW_ACTIVATE); //$NON-NLS-1$
         if (fullScreen)
         {
            ((ViewPart)view).setPartProperty("FullScreen", "true");
         }
         else
         {
            page.setPartState(page.getReference(view), IWorkbenchPage.STATE_MAXIMIZED);
         }
      }
      catch(PartInitException e)
      {
         MessageDialogHelper.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Error, String.format(Messages.get().ApplicationWorkbenchWindowAdvisor_CannotOpenDashboardType2, dashboardId, e.getLocalizedMessage()));
      }
   }

   /**
    * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#postWindowClose()
    */
   @Override
   public void postWindowClose()
   {
      super.postWindowClose();
      if (RWT.getUISession().getAttribute("NoPageReload") == null)
      {
         RWT.getClient().getService(ExitConfirmation.class).setMessage(null);
         JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
         if (executor != null)
            executor.execute("location.reload(true);");
      }
   }

   /**
    * Show the message of the day message box
    */
   private void showMessageOfTheDay()
   {   
      NXCSession session = ConsoleSharedData.getSession();

      String message = session.getMessageOfTheDay();
      if (!message.isEmpty())
      {
         MessageDialog.openInformation(Display.getCurrent().getActiveShell(), "Announcement", message);
      }

      LicenseProblem[] licenseProblems = session.getLicenseProblems();
      if ((licenseProblems != null) && (licenseProblems.length > 0))
      {
         StringBuilder sb = new StringBuilder();
         for(LicenseProblem p : licenseProblems)
         {
            if (sb.length() == 0)
               sb.append("\r\n");
            sb.append(p.getDescription());
         }
         MessageDialog.openWarning(Display.getCurrent().getActiveShell(), "License Problem", sb.toString());
      }
   }
}
