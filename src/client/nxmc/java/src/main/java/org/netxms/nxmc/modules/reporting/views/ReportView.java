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
package org.netxms.nxmc.modules.reporting.views;

import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.reporting.ReportDefinition;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewWithContext;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.reporting.widgets.ReportExecutionForm;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Report view
 */
public class ReportView extends ViewWithContext
{
   private final I18n i18n = LocalizationHelper.getI18n(ReportView.class);
   
   private Composite content;
   private ReportExecutionForm form;
   private Action actionExecuteReport;
   private Action actionScheduleExecution;

   /**
    * Create report view
    */
   public ReportView()
   {
      super("Report", ResourceManager.getImageDescriptor("icons/report.png"), "Report", false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      content = new Composite(parent, SWT.NONE);
      content.setLayout(new FillLayout());

      actionExecuteReport = new Action("E&xecute report", SharedIcons.EXECUTE) {
         @Override
         public void run()
         {
            form.executeReport();
         }
      };
      addKeyBinding("F9", actionExecuteReport);

      actionScheduleExecution = new Action("&Schedule report execution...", SharedIcons.CALENDAR) {
         @Override
         public void run()
         {
            form.scheduleExecution();
         }
      };
      addKeyBinding("M1+S", actionScheduleExecution);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExecuteReport);
      manager.add(actionScheduleExecution);
      super.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExecuteReport);
      manager.add(actionScheduleExecution);
      super.fillLocalMenu(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#contextChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {
      if (form != null)
         form.dispose();

      if ((newContext != null) && (newContext instanceof ReportDefinition))
         form = new ReportExecutionForm(content, SWT.NONE, (ReportDefinition)newContext, this);
      else
         form = null;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getFullName()
    */
   @Override
   public String getFullName()
   {
      return getContextName();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getContextName()
    */
   @Override
   protected String getContextName()
   {
      Object context = getContext();
      return ((context != null) && (context instanceof ReportDefinition)) ? ((ReportDefinition)context).getName() : "";
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("context", ((ReportDefinition)getContext()).getId());
   }
   
   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreContext(org.netxms.nxmc.Memento)
    */
   @Override
   public Object restoreContext(Memento memento)
   {      
      final UUID guid = memento.getAsUUID("context");
      if (guid == null)
         return null;
      final NXCSession session = Registry.getSession();
      ReportDefinition[] definition = new ReportDefinition[1];
      new Job(i18n.tr("Loading report definition"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(UUID reportId : session.listReports())
            {
               if (guid.equals(reportId))
               {
                  definition[0] = session.getReportDefinition(reportId);       
                  break;
               }
               
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Error loading report definition");
         }
      }.runInForeground();
      return definition[0];
   } 
}
