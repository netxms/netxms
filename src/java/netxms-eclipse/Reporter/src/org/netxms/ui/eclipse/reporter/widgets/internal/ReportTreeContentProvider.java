package org.netxms.ui.eclipse.reporter.widgets.internal;

import java.util.List;
import org.eclipse.jface.viewers.TreeNodeContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.api.client.reporting.ReportDefinition;

public class ReportTreeContentProvider extends TreeNodeContentProvider
{
	private List<ReportDefinition> definitions;

	@Override
	public Object[] getChildren(Object parentElement)
	{
		return null;
	}

	@Override
	public Object[] getElements(Object inputElement)
	{
		return definitions.toArray();
	}

	@Override
	public Object getParent(Object element)
	{
		return null;
	}

	@Override
	public boolean hasChildren(Object element)
	{
		return false;
	}

	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		this.definitions = (List<ReportDefinition>)newInput;
	}

}
