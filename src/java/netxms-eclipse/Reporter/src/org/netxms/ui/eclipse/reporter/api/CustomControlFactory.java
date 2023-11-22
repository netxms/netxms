package org.netxms.ui.eclipse.reporter.api;

import org.eclipse.swt.widgets.Composite;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.ui.eclipse.reporter.widgets.FieldEditor;

public interface CustomControlFactory
{
   public FieldEditor editorForType(Composite parent, ReportParameter parameter);
}
