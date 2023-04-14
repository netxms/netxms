/**
 * NetXMS - open source network management system
 * Copyright (C) 2023 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.dialogs;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
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
import org.netxms.client.asset.AssetAttribute;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Dialog for selecting asset attributes definition
 */
public class SelectAssetAttributeDlg extends Dialog 
{
   private TableViewer viewer;
   private List<AssetAttribute> selection;

   /**
    * @param parentShell
    * @param multiSelection
    */
   public SelectAssetAttributeDlg(Shell parentShell)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
   }

   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      Point size = getShell().getSize();
      Point location = getShell().getLocation();
      PreferenceStore settings = PreferenceStore.getInstance();

      settings.set("SelectAssetAttributeDlg.location", location); 
      settings.set("SelectAssetAttributeDlg.size", size); 
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Asset Attribute Definition Selection");
      PreferenceStore settings = PreferenceStore.getInstance();
      newShell.setSize(settings.getAsPoint("SelectAssetAttributeDlg.size", 400, 250)); 
      newShell.setLocation(settings.getAsPoint("SelectAssetAttributeDlg.location", 100, 100));
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
      
      viewer = new TableViewer(dialogArea, SWT.FULL_SELECTION | SWT.BORDER | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((AssetAttribute)element).getName();
         }
      });
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((AssetAttribute)e1).getName().compareToIgnoreCase(((AssetAttribute)e2).getName());
         }
      });
      
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 400;
      viewer.getControl().setLayoutData(gd);
      
      getAssetManagementAttributes();
      
      return dialogArea;
   }

   /**
    * Get all web service definitions
    */
   private void getAssetManagementAttributes()
   {
      final NXCSession session = Registry.getSession();
      Job job = new Job("Get web service definitions", null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Map<String, AssetAttribute> definitions = session.getAssetManagementSchema();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(definitions.values().toArray());
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
      IStructuredSelection viewerSelection = viewer.getStructuredSelection();
      if (viewerSelection.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Web service definition should be selected");
         return;
      }

      selection = new ArrayList<AssetAttribute>();
      for(Object o : viewerSelection.toList())
         selection.add((AssetAttribute)o);

      saveSettings();
      super.okPressed();
   }

   /**
    * Get selection list 
    * 
    * @return
    */
   public List<AssetAttribute> getSelection()
   {      
      return selection;
   }
}
