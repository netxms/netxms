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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.api.client.reporting.ReportParameter;

/**
 * Numeric field editor
 */
public class NumberFieldEditor extends FieldEditor
{
   private Spinner value;
   
   /**
    * @param parameter
    * @param toolkit
    * @param parent
    */
   public NumberFieldEditor(ReportParameter parameter, FormToolkit toolkit, Composite parent)
   {
      super(parameter, toolkit, parent);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContent(Composite parent)
   {
      value = new Spinner(parent, SWT.BORDER);
      value.setSelection(0);
      return value;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#getValue()
    */
   @Override
   public String getValue()
   {
      return Integer.toString(value.getSelection());
   }
}
