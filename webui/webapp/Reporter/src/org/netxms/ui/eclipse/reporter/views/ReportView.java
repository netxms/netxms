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
package org.netxms.ui.eclipse.reporter.views;

import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.ISelectionListener;
import org.eclipse.ui.ISelectionService;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.reporting.ReportDefinition;
import org.netxms.ui.eclipse.reporter.widgets.ReportExecutionForm;

/**
 * Report view
 */
public class ReportView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.reporter.views.ReportView";

	private ReportExecutionForm executionForm;
	private ISelectionListener selectionListener;
	private ISelectionService selectionService;

	private Composite parentComposite;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		this.parentComposite = parent;

		selectionService = getSite().getWorkbenchWindow().getSelectionService();
		selectionListener = new ISelectionListener() {
			@Override
			public void selectionChanged(IWorkbenchPart part, ISelection selection)
			{
				if ((part instanceof ReportNavigator) && (selection instanceof IStructuredSelection) && !selection.isEmpty())
				{
					ReportDefinition object = (ReportDefinition)((IStructuredSelection)selection).getFirstElement();
					setObject(object);
				}
			}
		};
		selectionService.addSelectionListener(selectionListener);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if ((selectionService != null) && (selectionListener != null))
		{
			selectionService.removeSelectionListener(selectionListener);
		}
		super.dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		if (executionForm != null)
		{
			executionForm.setFocus();
		}
	}

	/**
	 * @param definition
	 */
	protected void setObject(ReportDefinition definition)
	{
		if (executionForm != null)
		{
			executionForm.dispose();
		}

		executionForm = new ReportExecutionForm(parentComposite, SWT.NONE, definition, this);

		parentComposite.layout();
		setPartName(definition.getName());
	}
}
