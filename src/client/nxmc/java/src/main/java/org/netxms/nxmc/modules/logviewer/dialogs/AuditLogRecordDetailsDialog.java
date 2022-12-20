/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.logviewer.dialogs;

import java.util.LinkedList;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.DiffMatchPatch;
import org.netxms.base.DiffMatchPatch.Diff;
import org.netxms.client.log.LogRecordDetails;
import org.netxms.nxmc.base.widgets.DiffViewer;
import org.netxms.nxmc.base.widgets.StyledText;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Dialog for displaying audit log record details
 */
public class AuditLogRecordDetailsDialog extends Dialog
{
   private LogRecordDetails data;
   private CTabFolder tabFolder;

   /**
    * Create dialog
    * 
    * @param parentShell parent shell
    * @param data audit log record details
    */
   public AuditLogRecordDetailsDialog(Shell parentShell, LogRecordDetails data)
   {
      super(parentShell);
      this.data = data;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Audit Log Record Details");
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#isResizable()
    */
   @Override
   protected boolean isResizable()
   {
      return true;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, IDialogConstants.OK_ID, "Close", true);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      dialogArea.setLayout(layout);

      tabFolder = new CTabFolder(dialogArea, SWT.BORDER);
      tabFolder.setTabHeight(24);
      WidgetHelper.disableTabFolderSelectionBar(tabFolder);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 800;
      gd.heightHint = 500;
      tabFolder.setLayoutData(gd);

      String oldValue = data.getValue("old_value");
      String newValue = data.getValue("new_value");

      DiffMatchPatch diffEngine = new DiffMatchPatch();
      LinkedList<Diff> diffs = diffEngine.diff_main(oldValue, newValue);
      diffEngine.diff_cleanupSemantic(diffs);
      diffEngine.diff_prettyHtml(diffs);

      createTextValueTab("Old value", "icons/old_value.png", oldValue);
      createTextValueTab("New value", "icons/new_value.png", newValue);
      createDiffTab(diffs);

      tabFolder.setSelection(2);

      return dialogArea;
   }

   /**
    * Create text value tab
    * 
    * @param name
    * @param imageName
    * @param text
    */
   private void createTextValueTab(String name, String imageName, String text)
   {
      CTabItem tab = createTab(name, imageName);
      // Use StyledText instead of Text for uniform look with "Diff" tab
      StyledText textControl = new StyledText(tabFolder, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY);
      textControl.setFont(JFaceResources.getTextFont());
      textControl.setText(text);
      tab.setControl(textControl);
   }

   /**
    * Create tab with diff viewer
    * 
    * @param diffs
    */
   private void createDiffTab(LinkedList<Diff> diffs)
   {
      CTabItem tab = createTab("Diff", "icons/diff.png");
      DiffViewer viewer = new DiffViewer(tabFolder, SWT.NONE);
      viewer.setContent(diffs);
      tab.setControl(viewer);
   }

   /**
    * Create tab
    * 
    * @param name
    * @param imageName
    * @return
    */
   private CTabItem createTab(String name, String imageName)
   {
      CTabItem tab = new CTabItem(tabFolder, SWT.NONE);
      tab.setText(name);
      final Image image = ResourceManager.getImage(imageName);
      tab.setImage(image);
      tab.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            image.dispose();
         }
      });
      return tab;
   }
}
