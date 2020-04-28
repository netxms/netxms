/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.dialogs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.WebServiceDefinition;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Dialog for selecting web service definition
 */
public class SelectWebServiceDlg extends Dialog implements IParameterSelectionDialog
{
   boolean multiSelection;
   private TableViewer viewer;
   private List<WebServiceDefinition> selection;

   /**
    * @param parentShell
    * @param multiSelection
    */
   public SelectWebServiceDlg(Shell parentShell, boolean multiSelection)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.multiSelection = multiSelection;
   }

   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      Point size = getShell().getSize();
      Point location = getShell().getLocation();
      IDialogSettings settings = Activator.getDefault().getDialogSettings();

      settings.put("SelectWebServiceDlg.cx", location.x); //$NON-NLS-1$
      settings.put("SelectWebServiceDlg.cy", location.y); //$NON-NLS-1$
      settings.put("SelectWebServiceDlg.width", size.x); //$NON-NLS-1$
      settings.put("SelectWebServiceDlg.height", size.y); //$NON-NLS-1$
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Web Service Defenition Selection");
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      try
      {
         newShell.setSize(settings.getInt("SelectWebServiceDlg.width"), settings.getInt("SelectWebServiceDlg.height")); //$NON-NLS-1$ //$NON-NLS-2$
         newShell.setLocation(settings.getInt("SelectWebServiceDlg.cx"), settings.getInt("SelectWebServiceDlg.cy")); //$NON-NLS-1$ //$NON-NLS-2$
      }
      catch(NumberFormatException e)
      {
         newShell.setSize(400, 250);
         newShell.setLocation(100, 100);
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      final Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);
      
      viewer = new TableViewer(dialogArea, SWT.FULL_SELECTION | SWT.BORDER | (multiSelection ? SWT.MULTI : SWT.NONE));
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((WebServiceDefinition)element).getName();
         }
      });
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((WebServiceDefinition)e1).getName().compareToIgnoreCase(((WebServiceDefinition)e2).getName());
         }
      });
      
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 400;
      viewer.getControl().setLayoutData(gd);
      
      getWebServiceDefinitions();
      
      return dialogArea;
   }

   /**
    * Get all web service definitions
    */
   private void getWebServiceDefinitions()
   {
      final NXCSession session = ConsoleSharedData.getSession();
      ConsoleJob job = new ConsoleJob("Get web service definitions", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<WebServiceDefinition> definitions = session.getWebServiceDefinitions();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(definitions.toArray());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get web service definitions";
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
   @Override
   protected void cancelPressed()
   {
      saveSettings();
      super.cancelPressed();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      IStructuredSelection viewerSelection = (IStructuredSelection)viewer.getSelection();
      if (viewerSelection.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Web service definition should be selected");
         return;
      }

      selection = new ArrayList<WebServiceDefinition>();
      for(Object o : viewerSelection.toList())
         selection.add((WebServiceDefinition)o);

      saveSettings();
      super.okPressed();
   }

   /**
    * Get selection list 
    * 
    * @return
    */
   public List<WebServiceDefinition> getSelection()
   {      
      return selection;
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterName()
    */
   @Override
   public String getParameterName()
   {
      return selection.get(0).getName() + ":";
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDescription()
    */
   @Override
   public String getParameterDescription()
   {      
      return selection.get(0).getName();
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDataType()
    */
   @Override
   public DataType getParameterDataType()
   {
      return DataType.STRING;
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getInstanceColumn()
    */
   @Override
   public String getInstanceColumn()
   {
      return "";
   }
}
