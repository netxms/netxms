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
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.api.client.reporting.ReportParameter;
import org.netxms.client.constants.Severity;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;

/**
 * Severity list field editor
 */
public class SeverityListFieldEditor extends FieldEditor
{
   private Button[] severity;
   
   /**
    * @param parameter
    * @param toolkit
    * @param parent
    */
   public SeverityListFieldEditor(ReportParameter parameter, FormToolkit toolkit, Composite parent)
   {
      super(parameter, toolkit, parent);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContent(Composite parent)
   {
      Composite content = toolkit.createComposite(parent, SWT.BORDER);
      
      content.setLayout(new GridLayout());
      
      severity = new Button[5];
      for(int i = 0; i < Severity.UNKNOWN.getValue(); i++)
      {
         severity[i] = toolkit.createButton(content, StatusDisplayInfo.getStatusText(i), SWT.CHECK);
         severity[i].setSelection(true);
      }
      
      return content;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#getValue()
    */
   @Override
   public String getValue()
   {
      StringBuilder sb = new StringBuilder();
      for(int i = 0; i < Severity.UNKNOWN.getValue(); i++)
      {
         if (severity[i].getSelection())
         {
            if (sb.length() > 0)
               sb.append(',');
            sb.append(i);
         }
      }
      return sb.toString();
   }
}
