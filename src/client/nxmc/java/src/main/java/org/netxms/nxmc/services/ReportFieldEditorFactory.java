package org.netxms.nxmc.services;

import org.eclipse.swt.widgets.Composite;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.nxmc.modules.reporting.widgets.ReportFieldEditor;

public interface ReportFieldEditorFactory
{
   public ReportFieldEditor editorForType(Composite parent, ReportParameter parameter);

   /**
    * Get factory priority
    *
    * @return factory priority
    */
   public int getPriority();
}
