/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.InputField;
import org.netxms.nxmc.base.widgets.LabeledControl;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * This dialog collects input for provided input fields.
 */
public class InputFieldEntryDialog extends Dialog
{
   private String title;
   private InputField[] fields;
   private LabeledControl[] controls;
   private Map<String, String> values = null;

   /**
    * Helper method for reading input fields.
    * 
    * @param parentShell parent shell
    * @param title dialog title
    * @param fields input field set
    * @return set of read values or null if user cancelled dialog
    */
   public static Map<String, String> readInputFields(Shell parentShell, String title, InputField[] fields)
   {
      InputFieldEntryDialog dlg = new InputFieldEntryDialog(parentShell, title, fields);
      if (dlg.open() != Window.OK)
         return null;
      return dlg.getValues();
   }

   /**
    * @param parentShell
    * @param fields
    */
   public InputFieldEntryDialog(Shell parentShell, String title, InputField[] fields)
   {
      super(parentShell);
      this.title = title;
      this.fields = fields;
      this.controls = new LabeledControl[fields.length];
   }

   public InputFieldEntryDialog(Shell parentShell, String title, InputField[] fields, Map<String, String> inputValues)
   {
      super(parentShell);
      this.title = title;
      this.fields = fields;
      this.controls = new LabeledControl[inputValues.size()];
      values = inputValues;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(title);
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

      for(int i = 0; i < fields.length; i++)
      {
         LabeledControl c;
         switch(fields[i].getType())
         {
            case NUMBER:
               c = new LabeledSpinner(dialogArea, SWT.NONE);
               break;
            case PASSWORD:
               c = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.PASSWORD, 300);
               break;
            default:
               c = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER, 300);
               break;
         }
         c.setLabel(fields[i].getDisplayName());

         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         c.setLayoutData(gd);
         
         if (values != null)
         {
            if (values.containsKey(fields[i].getName()))
            {
               c.setText(values.get(fields[i].getName()));
            }
         }

         controls[i] = c;
      }

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      values = new HashMap<String, String>(); 
      for(int i = 0; i < fields.length; i++)
      {
         values.put(fields[i].getName(), controls[i].getText());
      }
      super.okPressed();
   }

   /**
    * @return the values
    */
   public Map<String, String> getValues()
   {
      return values;
   }
}
