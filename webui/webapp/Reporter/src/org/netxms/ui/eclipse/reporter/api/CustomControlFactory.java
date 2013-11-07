package org.netxms.ui.eclipse.reporter.api;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.api.client.reporting.ReportParameter;
import org.netxms.ui.eclipse.reporter.widgets.FieldEditor;

public interface CustomControlFactory
{

	FieldEditor editorForType(Composite parent, ReportParameter parameter, FormToolkit toolkit);

}
