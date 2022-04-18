/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 Raden Solutions
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
package org.netxms.nxmc.modules.alarms.dialogs;

import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.events.AlarmCategory;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.modules.alarms.widgets.AlarmCategoryList;

/**
 * Dialog for selecting alarm category
 */
public class AlarmCategorySelectionDialog extends Dialog
{
   private static final String TABLE_CONFIG_PREFIX = "AlarmCategorySelectionDialog";

   private AlarmCategoryList alarmCategoryList;
   private PreferenceStore settings;
   private AlarmCategory selectedEvents[];

   /**
    * Create new dialog
    *
    * @param parentShell parent shell
    */
   public AlarmCategorySelectionDialog(Shell parentShell)
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
      newShell.setText("Select Alarm Category");
      settings = PreferenceStore.getInstance();
      newShell.setSize(settings.getAsInteger(TABLE_CONFIG_PREFIX + ".cx", 600), settings.getAsInteger(TABLE_CONFIG_PREFIX + ".cy", 400));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      dialogArea.setLayout(new FormLayout());

      alarmCategoryList = new AlarmCategoryList(dialogArea, SWT.NONE, TABLE_CONFIG_PREFIX, false);
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      alarmCategoryList.setLayoutData(fd);

      alarmCategoryList.getViewer().addDoubleClickListener(new IDoubleClickListener() {         
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            okPressed();
         }
      });
      
      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   protected void createButtonsForButtonBar(Composite parent)
   {
      // Change parent layout data to fill the whole bar
      parent.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Button newButton = createButton(parent, IDialogConstants.NO_ID, "&New...", false);
      newButton.setLayoutData(new GridData(SWT.LEFT, SWT.FILL, true, false));
      newButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            alarmCategoryList.createCategory();
         }
      });

      // Create a spacer label
      Label spacer = new Label(parent, SWT.NONE);
      spacer.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Update layout of the parent composite to count the spacer
      GridLayout layout = (GridLayout)parent.getLayout();
      layout.numColumns++;
      layout.makeColumnsEqualWidth = false;

      super.createButtonsForButtonBar(parent);
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
   @SuppressWarnings("unchecked")
   @Override
   protected void okPressed()
   {
      final IStructuredSelection selection = alarmCategoryList.getSelection();
      final List<AlarmCategory> list = selection.toList();
      selectedEvents = list.toArray(new AlarmCategory[list.size()]);
      saveSettings();
      super.okPressed();
   }

   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      Point size = getShell().getSize();
      settings.set(TABLE_CONFIG_PREFIX + ".cx", size.x);
      settings.set(TABLE_CONFIG_PREFIX + ".cy", size.y);
   }

   /**
    * Get selected alarm categories
    * 
    * @return Selected alarm categories
    */
   public AlarmCategory[] getSelectedCategories()
   {
      return selectedEvents;
   }
}
