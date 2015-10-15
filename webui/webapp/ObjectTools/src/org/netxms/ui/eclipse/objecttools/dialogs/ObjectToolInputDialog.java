/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objecttools.dialogs;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objecttools.InputField;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledControl;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Object tool input dialog - collects input for all configured input fields.
 */
public class ObjectToolInputDialog extends Dialog
{
   private InputField[] fields;
   private LabeledControl[] controls;
   private Map<String, String> values;
   
   /**
    * @param parentShell
    * @param fields
    */
   public ObjectToolInputDialog(Shell parentShell, InputField[] fields)
   {
      super(parentShell);
      this.fields = fields;
      this.controls = new LabeledControl[fields.length];
   }
   
   /* (non-Javadoc)
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
               c = new LabeledText(dialogArea, SWT.NONE, SWT.PASSWORD);
               break;
            default:
               c = new LabeledText(dialogArea, SWT.NONE);
               break;
         }
         c.setLabel(fields[i].getDisplayName());
         
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.widthHint = 300;
         c.setLayoutData(gd);
         
         controls[i] = c;
      }

      return dialogArea;
   }

   /* (non-Javadoc)
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
