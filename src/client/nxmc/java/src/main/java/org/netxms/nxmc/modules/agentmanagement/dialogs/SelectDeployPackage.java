/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Reden Solutions
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
package org.netxms.nxmc.modules.agentmanagement.dialogs;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.packages.PackageInfo;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.views.PackageManager;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.PackageComparator;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.PackageLabelProvider;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog to select package for deployment
 */
public class SelectDeployPackage extends Dialog
{
   private static I18n i18n = LocalizationHelper.getI18n(SelectDeployPackage.class);
   private TableViewer packageList;
   private NXCSession session;
   private long packageId;

   public SelectDeployPackage(Shell parentShell)
   {
      super(parentShell);
      session = Registry.getSession();
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Select Deployment Package"));
   }

   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);  
      
      final String[] names = { i18n.tr("ID"), i18n.tr("Name"), i18n.tr("Type"),
            i18n.tr("Version"), i18n.tr("Platform"),
            i18n.tr("File"), "Command", i18n.tr("Description") };
      final int[] widths = { 70, 120, 100, 90, 120, 300, 300, 400 };
      packageList = new SortableTableViewer(parent, names, widths, PackageManager.COLUMN_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      packageList.setContentProvider(new ArrayContentProvider());
      packageList.setLabelProvider(new PackageLabelProvider());
      packageList.setComparator(new PackageComparator());
      
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 300;
      gd.widthHint = 600;
      packageList.getControl().setLayoutData(gd);
      
      return dialogArea;
   }

   @Override
   protected Control createContents(Composite parent)
   {
      Control content = super.createContents(parent);
      // must be called from createContents to make sure that buttons already created
      getPackagesAndRefresh();
      return content;
   }
   
   private void getPackagesAndRefresh()
   {
      packageList.setInput(new String[] { "Loading..." });
      getButton(IDialogConstants.OK_ID).setEnabled(false);

      Job job = new Job(i18n.tr("Synchronize users"), null, null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<PackageInfo> list = session.getInstalledPackages();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {                      
                  packageList.setInput(list);
                  getButton(IDialogConstants.OK_ID).setEnabled(true);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot synchronize users";
         }
      };
      job.setUser(false);
      job.start();
   }

   @Override
   protected void okPressed()
   {
      IStructuredSelection sel = (IStructuredSelection)packageList.getSelection();
   
      if (sel.size() != 1)
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"),
               i18n.tr("You must select one package from the list and then press OK."));
         return;
      }
      packageId = ((PackageInfo)sel.getFirstElement()).getId();
      super.okPressed();
   }
   
   public long getSelectedPackageId()
   {
      return packageId;
   }
}
