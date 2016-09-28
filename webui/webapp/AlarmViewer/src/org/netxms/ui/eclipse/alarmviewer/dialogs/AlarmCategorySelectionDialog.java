/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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
package org.netxms.ui.eclipse.alarmviewer.dialogs;

import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.events.AlarmCategory;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmCategoryList;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmCategoryListFilter;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;

public class AlarmCategorySelectionDialog extends Dialog
{
   public static final String JOB_FAMILY = "AlarmCategorySelectorJob"; //$NON-NLS-1$
   private static final String TABLE_CONFIG_PREFIX = "AlarmCategorySelectionDialog"; //$NON-NLS-1$

   private AlarmCategoryList alarmCategoryList;
   private FilterText filterText;
   private AlarmCategoryListFilter filter;
   private IDialogSettings settings;
   private AlarmCategory selectedEvents[];

   public AlarmCategorySelectionDialog(Shell parentShell)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Select category");
      settings = Activator.getDefault().getDialogSettings();
      try
      {
         newShell.setSize(settings.getInt(TABLE_CONFIG_PREFIX + ".cx"), settings.getInt(TABLE_CONFIG_PREFIX + ".cy"));
      }
      catch(NumberFormatException e)
      {
      }
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      // Create filter area
      filterText = new FilterText(dialogArea, SWT.NONE, null, false);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      filterText.setLayoutData(gd);

      alarmCategoryList = new AlarmCategoryList(dialogArea, SWT.NONE, TABLE_CONFIG_PREFIX, false);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 350;
      alarmCategoryList.setLayoutData(gd);

      alarmCategoryList.getViewer().addDoubleClickListener(new IDoubleClickListener() {         
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            okPressed();
         }
      });
      
      return dialogArea;
   }

   protected void createButtonsForButtonBar(Composite parent)
   {
      // Change parent layout data to fill the whole bar
      parent.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Button newButton = createButton(parent, IDialogConstants.NO_ID, "New category...", false);
      newButton.setLayoutData(new GridData(SWT.LEFT, SWT.FILL, true, false));
      newButton.addMouseListener(new MouseListener() {

         @Override
         public void mouseUp(MouseEvent e)
         {
            alarmCategoryList.createCategory();
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
         }

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
         }
      });

      // Create a spacer label
      Label spacer = new Label(parent, SWT.NONE);
      spacer.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Update layout of the parent composite to count the spacer
      GridLayout layout = (GridLayout)parent.getLayout();
      layout.numColumns++;
      layout.makeColumnsEqualWidth = false;

      createButton(parent, IDialogConstants.CANCEL_ID, "Cancel", true);
      createButton(parent, IDialogConstants.OK_ID, "OK", true);
   }

   @Override
   protected void cancelPressed()
   {
      saveSettings();
      super.cancelPressed();
   }

   @SuppressWarnings("unchecked")
   @Override
   protected void okPressed()
   {
      final IStructuredSelection selection = (IStructuredSelection)alarmCategoryList.getSelection();
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

      settings.put(TABLE_CONFIG_PREFIX + ".cx", size.x); //$NON-NLS-1$
      settings.put(TABLE_CONFIG_PREFIX + ".cy", size.y); //$NON-NLS-1$
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
   
   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      alarmCategoryList.getViewer().refresh(false);
   }

}
