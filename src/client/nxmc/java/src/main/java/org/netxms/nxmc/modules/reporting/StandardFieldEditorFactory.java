/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.reporting;

import org.eclipse.swt.widgets.Composite;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.nxmc.modules.reporting.widgets.AlarmStateFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.BooleanFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.DateFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.EventFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.MultiSelectFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.NumberFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.NumericConditionFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.ObjectFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.ObjectListFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.ReportFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.SeverityFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.SeverityListFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.StringFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.TimestampFieldEditor;
import org.netxms.nxmc.modules.reporting.widgets.UserFieldEditor;
import org.netxms.nxmc.services.ReportFieldEditorFactory;

/**
 * Control factory for standard control types
 */
public class StandardFieldEditorFactory implements ReportFieldEditorFactory
{
   /**
    * @see org.netxms.nxmc.services.ReportFieldEditorFactory#editorForType(org.eclipse.swt.widgets.Composite,
    *      org.netxms.client.reporting.ReportParameter)
    */
	@Override
   public ReportFieldEditor editorForType(Composite parent, ReportParameter parameter)
	{
      ReportFieldEditor fieldEditor = null;
		final String type = parameter.getType();
      if (type.equals("START_DATE") || type.equals("END_DATE"))
		{
         fieldEditor = new DateFieldEditor(parameter, parent);
		}
      else if (type.equals("ALARM_STATE"))
      {
         fieldEditor = new AlarmStateFieldEditor(parameter, parent);
      }
      else if (type.equals("BOOLEAN"))
      {
         fieldEditor = new BooleanFieldEditor(parameter, parent);
      }
      else if (type.equals("EVENT_CODE"))
      {
         fieldEditor = new EventFieldEditor(parameter, parent);
      }
      else if (type.equals("MULTISELECT"))
      {
         fieldEditor = new MultiSelectFieldEditor(parameter, parent);
      }
      else if (type.equals("NUMBER"))
      {
         fieldEditor = new NumberFieldEditor(parameter, parent);
      }
      else if (type.equals("NUMERIC_CONDITION"))
      {
         fieldEditor = new NumericConditionFieldEditor(parameter, parent);
      }
      else if (type.equals("OBJECT_ID"))
      {
         fieldEditor = new ObjectFieldEditor(parameter, parent);
      }
      else if (type.equals("OBJECT_ID_LIST"))
      {
         fieldEditor = new ObjectListFieldEditor(parameter, parent);
      }
      else if (type.equals("SEVERITY"))
      {
         fieldEditor = new SeverityFieldEditor(parameter, parent);
      }
      else if (type.equals("SEVERITY_LIST"))
      {
         fieldEditor = new SeverityListFieldEditor(parameter, parent);
      }
      else if (type.equals("TIMESTAMP"))
		{
         fieldEditor = new TimestampFieldEditor(parameter, parent);
		}
      else if (type.equals("USER_ID"))
		{
         fieldEditor = new UserFieldEditor(parameter, parent, false);
		}
      else if (type.equals("USER_NAME"))
		{
         fieldEditor = new UserFieldEditor(parameter, parent, true);
		}
		else
		{
         fieldEditor = new StringFieldEditor(parameter, parent);
		}
		return fieldEditor;
	}

   /**
    * @see org.netxms.nxmc.services.ReportFieldEditorFactory#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 0;
   }
}
