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
package org.netxms.nxmc.modules.datacollection.dialogs;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.AgentParameter;
import org.netxms.client.AgentTable;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Template;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for selecting parameters/tables provided by NetXMS agent or device driver
 */
public class SelectAgentParamDlg extends AbstractSelectParamDlg
{
   private final I18n i18n = LocalizationHelper.getI18n(SelectAgentParamDlg.class);
   
   private DataOrigin origin;
   private Button queryButton;
   private Action actionQuery;
   private AbstractObject queryObject;

   /**
    * @param parentShell
    * @param nodeId
    */
   public SelectAgentParamDlg(Shell parentShell, long nodeId, DataOrigin origin, boolean selectTables)
   {
      super(parentShell, nodeId, selectTables);
      this.origin = origin;
      queryObject = object;

      actionQuery = new Action(i18n.tr("&Query...")) {
         @Override
         public void run()
         {
            querySelectedParameter();
         }
      };
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      if (!selectTables)
      {
         ((GridLayout)parent.getLayout()).numColumns++;

         queryButton = new Button(parent, SWT.PUSH);
         queryButton.setText(i18n.tr("&Query..."));
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         queryButton.setLayoutData(gd);
         queryButton.addSelectionListener(new SelectionListener() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               querySelectedParameter();
            }

            @Override
            public void widgetDefaultSelected(SelectionEvent e)
            {
               widgetSelected(e);
            }
         });
      }
      super.createButtonsForButtonBar(parent);
   }

   /*
    * (non-Javadoc)
    * 
    * @see
    * org.netxms.ui.eclipse.datacollection.dialogs.AbstractSelectParamDlg#fillContextMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillContextMenu(IMenuManager manager)
   {
      super.fillContextMenu(manager);
      if (!selectTables)
         manager.add(actionQuery);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.datacollection.dialogs.AbstractSelectParamDlg#fillList()
    */
   @Override
   protected void fillList()
   {
      final NXCSession session = Registry.getSession();
      new Job(String.format(i18n.tr("Get list of supported parameters for "), object.getObjectName()), null) {
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Unable to retrieve list of supported parameters");
         }

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if (selectTables)
            {
               final List<AgentTable> tables = session.getSupportedTables(object.getObjectId());
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.setInput(tables.toArray());
                  }
               });
            }
            else
            {
               final List<AgentParameter> parameters = session.getSupportedParameters(object.getObjectId(), origin);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.setInput(parameters.toArray());
                  }
               });
            }
         }
      }.start();
   }

   /**
    * Query current value of selected parameter
    */
   protected void querySelectedParameter()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;
      
      // Opens Object Selection Dialog if object is not chosen
      if (queryObject instanceof Template)
      {
         final ObjectSelectionDialog sDlg = new ObjectSelectionDialog(getShell(), ObjectSelectionDialog.createNodeSelectionFilter(false));
         sDlg.enableMultiSelection(false);
         
         if (sDlg.open() == Window.OK)
         {
            queryObject = sDlg.getSelectedObjects().get(0);
         }
      }

      AgentParameter p = (AgentParameter)selection.getFirstElement();
      String n;
      if (p.getName().contains("(*)")) //$NON-NLS-1$
      {
         InputDialog dlg = new InputDialog(getShell(), i18n.tr("Instance"),
               i18n.tr("Instance for parameter"), "", null); //$NON-NLS-1$
         if (dlg.open() != Window.OK)
            return;

         n = p.getName().replace("(*)", "(" + dlg.getValue() + ")"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
      }
      else
      {
         n = p.getName();
      }

      final NXCSession session = Registry.getSession();
      final String name = n;
      new Job(i18n.tr("Query agent"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final String value = session.queryMetric(queryObject.getObjectId(), origin, name);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     MessageDialogHelper.openInformation(getShell(), i18n.tr("Current value"),
                           String.format(i18n.tr("Current value is \"%s\""), value));
                  }
               });
            }
            catch (NXCException e)
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {

                     MessageDialogHelper.openInformation(getShell(), i18n.tr("Current value"),
                           String.format(i18n.tr("Cannot get current parameter value: %s"), e.getMessage()));
                     
                  }
               });
               throw e;
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get current parameter value");
         }
      }.start();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.datacollection.dialogs.AbstractSelectParamDlg#getConfigurationPrefix()
    */
   @Override
   protected String getConfigurationPrefix()
   {
      return (selectTables ? "SelectAgentTableDlg." : "SelectAgentParamDlg.") + origin; //$NON-NLS-1$ //$NON-NLS-2$
   }
}
