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
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.api.client.reporting.ReportParameter;
import org.netxms.ui.eclipse.reporter.Messages;

/**
 * Boolean field editor
 */
public class BooleanFieldEditor extends FieldEditor
{
   private Combo value;
   
   /**
    * @param parameter
    * @param toolkit
    * @param parent
    */
   public BooleanFieldEditor(ReportParameter parameter, FormToolkit toolkit, Composite parent)
   {
      super(parameter, toolkit, parent);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContent(Composite parent)
   {
      value = new Combo(parent, SWT.BORDER | SWT.DROP_DOWN | SWT.READ_ONLY);
      value.add(""); //$NON-NLS-1$
      value.add(Messages.get().BooleanFieldEditor_Yes);
      value.add(Messages.get().BooleanFieldEditor_No);
      value.select(0);
      return value;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#getValue()
    */
   @Override
   public String getValue()
   {
      int idx = value.getSelectionIndex();
      return Integer.toString((idx == 2) ? 0 : ((idx == 0) ? -1 : 1));
   }
}
