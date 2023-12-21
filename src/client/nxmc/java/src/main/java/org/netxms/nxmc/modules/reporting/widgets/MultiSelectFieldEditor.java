/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.reporting.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Numeric field editor
 */
public class MultiSelectFieldEditor extends ReportFieldEditor
{
   private List<Value> values = new ArrayList<Value>();
   private ValueSelector valueSelector;

   /**
    * @param parameter
    * @param toolkit
    * @param parent
    */
   public MultiSelectFieldEditor(ReportParameter parameter, Composite parent)
   {
      super(parameter, parent);

      if ((parameter.getMultiselectValues() != null) && !parameter.getMultiselectValues().isEmpty())
      {
         String[] parts = parameter.getMultiselectValues().split(";");
         for(String p : parts)
            values.add(new Value(p.trim()));
         valueSelector.updateSelectionText();
      }
   }

   /**
    * @see org.netxms.nxmc.modules.reporting.widgets.ReportFieldEditor#setupLocalization()
    */
   @Override
   protected void setupLocalization()
   {
   }

   /**
    * @see org.netxms.ReportFieldEditor.eclipse.reporter.widgets.FieldEditor#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContent(Composite parent)
   {
      valueSelector = new ValueSelector(parent);
      return valueSelector;
   }

   /**
    * @see org.netxms.ReportFieldEditor.eclipse.reporter.widgets.FieldEditor#getValue()
    */
   @Override
   public String getValue()
   {
      StringBuilder sb = new StringBuilder();
      for(Value v : values)
      {
         if (v.selected)
         {
            if (sb.length() > 0)
               sb.append(';');
            sb.append(v.value);
         }
      }
      return sb.toString();
   }

   /**
    * Value for selection
    */
   private static class Value
   {
      String displayName;
      String value;
      boolean selected;

      Value(String input)
      {
         String[] parts = input.split("=");
         if (parts.length > 1)
         {
            displayName = parts[0].trim();
            value = parts[1].trim();
         }
         else
         {
            displayName = value = input;
         }
         if (displayName.endsWith("[x]"))
         {
            displayName = displayName.substring(0, displayName.length() - 3).trim();
            selected = true;
         }
         else
         {
            selected = false;
         }
      }
   }

   /**
    * Value selector
    */
   private class ValueSelector extends AbstractSelector
   {
      public ValueSelector(Composite parent)
      {
         super(parent, SWT.NONE, new SelectorConfigurator().setShowLabel(false));
      }

      /**
       * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
       */
      @Override
      protected void selectionButtonHandler()
      {
         ValueSelectionDialog dlg = new ValueSelectionDialog(getShell());
         if (dlg.open() == Window.OK)
            updateSelectionText();
      }

      /**
       * Update selection text
       */
      public void updateSelectionText()
      {
         StringBuilder sb = new StringBuilder();
         for(Value v : values)
         {
            if (v.selected)
            {
               if (sb.length() > 0)
                  sb.append("; ");
               sb.append(v.displayName);
            }
         }
         setText(sb.toString());
      }
   }

   /**
    * Dialog for selecting values
    */
   private class ValueSelectionDialog extends Dialog
   {
      private Button[] selectors = new Button[values.size()];

      protected ValueSelectionDialog(Shell parentShell)
      {
         super(parentShell);
      }

      /**
       * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
       */
      @Override
      protected void configureShell(Shell newShell)
      {
         super.configureShell(newShell);
         newShell.setText(parameter.getDescription());
      }

      /**
       * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
       */
      @Override
      protected Control createDialogArea(Composite parent)
      {
         Composite dialogArea = (Composite)super.createDialogArea(parent);

         GridLayout layout = new GridLayout();
         layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
         layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
         dialogArea.setLayout(layout);

         for(int i = 0; i < values.size(); i++)
         {
            Value v = values.get(i);
            selectors[i] = new Button(dialogArea, SWT.CHECK);
            selectors[i].setText(v.displayName);
            selectors[i].setSelection(v.selected);
         }

         return dialogArea;
      }

      /**
       * @see org.eclipse.jface.dialogs.Dialog#okPressed()
       */
      @Override
      protected void okPressed()
      {
         for(int i = 0; i < selectors.length; i++)
            values.get(i).selected = selectors[i].getSelection();
         super.okPressed();
      }
   }
}
