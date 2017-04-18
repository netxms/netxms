/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.alarmviewer.views;

import java.io.IOException;
import java.net.URL;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.browser.IWebBrowser;
import org.netxms.client.NXCException;
import org.netxms.client.TableRow;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.events.Alarm;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.logviewer.views.LogViewer;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

public class AlarmLogViewer extends LogViewer
{
   public static final String ID = "org.netxms.ui.eclipse.alarmviewer.views.alarm_log_viewer"; //$NON-NLS-1$
   
   private Action actionCreateIssue;
   private Action actionShowIssue;
   private Action actionUnlinkIssue;
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.logviewer.views.LogViewer#createActions()
    */
   @Override
   protected void createActions()
   {
      super.createActions();
      actionCreateIssue = new Action("Create &ticket in helpdesk system", Activator.getImageDescriptor("icons/helpdesk_ticket.png")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            createIssue();
         }
      };
      
      actionShowIssue = new Action("Show helpdesk ticket in &web browser", SharedIcons.BROWSER) {
         @Override
         public void run()
         {
            showIssue();
         }
      };
      
      actionUnlinkIssue = new Action("Unlink from helpdesk ticket") {
         @Override
         public void run()
         {
            unlinkIssue();
         }
      };
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.logviewer.views.LogViewer#fillContextMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillContextMenu(final IMenuManager mgr)
   {
      super.fillContextMenu(mgr);
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (true)//selection.size() == 0 && session.isHelpdeskLinkActive())
      {
         try
         {
            Alarm alarm = session.getAlarm(Long.parseLong(((TableRow)selection.getFirstElement()).get(0).getValue()));
            if (alarm.getHelpdeskState() == Alarm.HELPDESK_STATE_IGNORED)
               mgr.add(actionCreateIssue);
            else
            {
               mgr.add(actionShowIssue);
               if ((session.getUserSystemRights() & UserAccessRights.SYSTEM_ACCESS_UNLINK_ISSUES) != 0)
                  mgr.add(actionUnlinkIssue);
            }
         }
         catch(NumberFormatException | ArrayIndexOutOfBoundsException | IOException | NXCException e)
         {
            Activator.logError("Error finding alarm", e);
         }
      }
   }
   
   /**
    * Create helpdesk ticket (issue) from selected alarms
    */
   private void createIssue()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;
      
      final long id = Long.parseLong(((TableRow)selection.getFirstElement()).get(0).getValue());
      new ConsoleJob("Create helpdesk ticket", this, Activator.PLUGIN_ID, JOB_FAMILY) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.openHelpdeskIssue(id);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot create helpdesk ticket from alarm";
         }
      }.start();
   }
   
   /**
    * Show in web browser helpdesk ticket (issue) linked to selected alarm
    */
   private void showIssue()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;
      
      final long id = Long.parseLong(((TableRow)selection.getFirstElement()).get(0).getValue());
      new ConsoleJob("Show helpdesk ticket", this, Activator.PLUGIN_ID, JOB_FAMILY) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final String url = session.getHelpdeskIssueUrl(id);
            runInUIThread(new Runnable() { 
               @Override
               public void run()
               {
                  try
                  {
                     final IWebBrowser browser = PlatformUI.getWorkbench().getBrowserSupport().getExternalBrowser();
                     browser.openURL(new URL(url));
                  }
                  catch(Exception e)
                  {
                     Activator.logError("Exception in AlarmList.showIssue (url=\"" + url + "\")", e); //$NON-NLS-1$ //$NON-NLS-2$
                     MessageDialogHelper.openError(getSite().getShell(), "Error", "Internal error: unable to open web browser");
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get URL for helpdesk ticket";
         }
      }.start();
   }
   
   /**
    * Unlink helpdesk ticket (issue) from selected alarm
    */
   private void unlinkIssue()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;

      final long id = Long.parseLong(((TableRow)selection.getFirstElement()).get(0).getValue());
      new ConsoleJob("Unlink alarm from helpdesk ticket", this, Activator.PLUGIN_ID, JOB_FAMILY) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.unlinkHelpdeskIssue(id);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot unlink alarm from helpdesk ticket";
         }
      }.start();
   }   
}