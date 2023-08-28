/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Reden Solutions
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
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.packages.PackageInfo;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.views.PackageManager;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.PackageComparator;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.PackageLabelProvider;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog to select package for deployment
 */
public class PackageSelectionDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(PackageSelectionDialog.class);

   private SortableTableViewer packageList;
   private long packageId;

   public PackageSelectionDialog(Shell parentShell)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Select Package"));
      PreferenceStore ps = PreferenceStore.getInstance();
      newShell.setSize(ps.getAsInteger("PackageSelectionDialog.w", 700), ps.getAsInteger("PackageSelectionDialog.h", 400));
      newShell.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            PreferenceStore ps = PreferenceStore.getInstance();
            Point s = getShell().getSize();
            ps.set("PackageSelectionDialog.w", s.x);
            ps.set("PackageSelectionDialog.h", s.y);
         }
      });
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      dialogArea.setLayout(new FillLayout());

      final String[] names = { i18n.tr("ID"), i18n.tr("Name"), i18n.tr("Type"), i18n.tr("Version"), i18n.tr("Platform"), i18n.tr("File"), "Command", i18n.tr("Description") };
      final int[] widths = { 70, 120, 100, 90, 120, 300, 300, 400 };
      packageList = new SortableTableViewer(dialogArea, names, widths, PackageManager.COLUMN_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      packageList.setContentProvider(new ArrayContentProvider());
      packageList.setLabelProvider(new PackageLabelProvider());
      packageList.setComparator(new PackageComparator());

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Control content = super.createContents(parent);
      // must be called from createContents to make sure that buttons already created
      fetchPackageList();
      return content;
   }

   /**
    * Fetch package list from server and update viewer
    */
   private void fetchPackageList()
   {
      packageList.setInput(new String[] { "Loading..." });
      getButton(IDialogConstants.OK_ID).setEnabled(false);

      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Fetching list of available packages"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<PackageInfo> list = session.getInstalledPackages();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {                      
                  packageList.setInput(list);
                  packageList.packColumns();
                  getButton(IDialogConstants.OK_ID).setEnabled(true);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot fetch list of available packages");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      IStructuredSelection selection = packageList.getStructuredSelection();
      if (selection.size() != 1)
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("You must select one package from the list and then press OK."));
         return;
      }

      packageId = ((PackageInfo)selection.getFirstElement()).getId();
      super.okPressed();
   }

   /**
    * Get ID of selected package.
    *
    * @return ID of selected package
    */
   public long getSelectedPackageId()
   {
      return packageId;
   }
}
