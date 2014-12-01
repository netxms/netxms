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
package org.netxms.ui.eclipse.reporter.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.api.client.reporting.ReportParameter;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Numeric condition field editor
 */
public class NumericConditionFieldEditor extends FieldEditor
{
   private Combo condition;
   private Spinner value;
   
   /**
    * @param parameter
    * @param toolkit
    * @param parent
    */
   public NumericConditionFieldEditor(ReportParameter parameter, FormToolkit toolkit, Composite parent)
   {
      super(parameter, toolkit, parent);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContent(Composite parent)
   {
      Composite content = toolkit.createComposite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      content.setLayout(layout);
      
      condition = new Combo(content, SWT.BORDER | SWT.DROP_DOWN | SWT.READ_ONLY);
      condition.add("="); //$NON-NLS-1$
      condition.add("<>"); //$NON-NLS-1$
      condition.add("<"); //$NON-NLS-1$
      condition.add("<="); //$NON-NLS-1$
      condition.add(">"); //$NON-NLS-1$
      condition.add(">="); //$NON-NLS-1$
      condition.select(0);
      condition.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, false, false));
      
      value = new Spinner(content, SWT.BORDER);
      value.setSelection(0);
      value.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));
      
      return content;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#getValue()
    */
   @Override
   public String getValue()
   {
      return condition.getText() + value.getText();
   }
}
