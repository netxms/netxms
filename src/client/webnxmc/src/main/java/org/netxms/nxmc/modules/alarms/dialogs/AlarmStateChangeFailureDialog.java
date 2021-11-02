/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2020 RadenSolutions
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

import java.util.Locale;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.constants.RCC;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Show alarm state change failures
 */
public class AlarmStateChangeFailureDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(AlarmStateChangeFailureDialog.class);
   private SortableTableViewer alarmList;
   private Map<Long, Integer> failures;

   /**
    * @param parentShell
    */
   public AlarmStateChangeFailureDialog(Shell parentShell, Map<Long, Integer> failures)
   {
      super(parentShell);
      this.failures = failures;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Alarm State Change Errors"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);
      
      Label text = new Label(dialogArea, SWT.NONE);
      text.setText(i18n.tr("State of some alarms was not changed due to following reasons:"));

      final String[] names = { i18n.tr("ID"), i18n.tr("Reason") };
      final int[] widths = { 80, 250 };
      alarmList = new SortableTableViewer(dialogArea, names, widths, 0, SWT.UP, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI | SWT.V_SCROLL);
      alarmList.setContentProvider(new MapContentProvider());
      alarmList.setLabelProvider(new FailuresLabelProvider());
      alarmList.setInput(failures);

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 350;
      alarmList.getTable().setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      super.okPressed();
   }
   
   /**
    * Label provider for failures map
    */
   private class FailuresLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         return (columnIndex == 0) ? ((Entry<?, ?>)element).getKey().toString() : RCC.getText((Integer)((Entry<?, ?>)element).getValue(), Locale.getDefault().getLanguage(), null);
      }
   }

   /**
    * Content provider for Map objects
    */
   private class MapContentProvider implements IStructuredContentProvider
   {
      @Override
      public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
      {
      }

      @SuppressWarnings("rawtypes")
      @Override
      public Object[] getElements(Object inputElement)
      {
         if (inputElement == null)
            return null;

         if (inputElement instanceof Map)
         {
            return ((Map)inputElement).entrySet().toArray();
         }

         return new Object[0];
      }

      @Override
      public void dispose()
      {
      }
   }
}
