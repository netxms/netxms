package org.netxms.ui.eclipse.reporter;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.api.client.reporting.ReportParameter;
import org.netxms.ui.eclipse.reporter.api.CustomControlFactory;
import org.netxms.ui.eclipse.reporter.widgets.FieldEditor;
import org.netxms.ui.eclipse.reporter.widgets.StringFieldEditor;
import org.netxms.ui.eclipse.reporter.widgets.TimestampFieldEditor;
import org.netxms.ui.eclipse.reporter.widgets.UserFieldEditor;

/**
 * Control factory for standard control types
 */
public class StandardTypesControlFactory implements CustomControlFactory
{
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.reporter.api.CustomControlFactory#editorForType(org.eclipse.swt.widgets.Composite, org.netxms.api.client.reporting.ReportParameter, org.eclipse.ui.forms.widgets.FormToolkit)
	 */
	@Override
	public FieldEditor editorForType(Composite parent, ReportParameter parameter, FormToolkit toolkit)
	{
		FieldEditor fieldEditor = null;
		final String type = parameter.getType();
		System.out.println(parameter);
		if (type.equals("START_DATE") || type.equals("END_DATE") || type.equals("TIMESTAMP"))
		{
			fieldEditor = new TimestampFieldEditor(parameter, toolkit, parent);
		}
		else if (type.equals("USER_ID"))
		{
			fieldEditor = new UserFieldEditor(parameter, toolkit, parent, false);
		}
		else if (type.equals("USER_NAME"))
		{
			fieldEditor = new UserFieldEditor(parameter, toolkit, parent, true);
		}
		else if (type.equals("OBJECT_ID"))
		{
		}
		else
		{
			fieldEditor = new StringFieldEditor(parameter, toolkit, parent);
		}
		return fieldEditor;
	}
}
