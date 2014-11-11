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
package org.netxms.ui.eclipse.reporter.widgets.helpers;

import java.text.DateFormat;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.api.client.reporting.ReportResult;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.reporter.Messages;
import org.netxms.ui.eclipse.reporter.widgets.ReportExecutionForm;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for report result list
 */
public class ReportResultLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private DateFormat dateFormat = RegionalSettings.getDateTimeFormat();

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		final ReportResult reportResult = (ReportResult)element;
		switch(columnIndex)
		{
			case ReportExecutionForm.RESULT_EXEC_TIME:
				return dateFormat.format(reportResult.getExecutionTime());
			case ReportExecutionForm.RESULT_STARTED_BY:
            AbstractUserObject user = ((NXCSession)ConsoleSharedData.getSession()).findUserDBObjectById(reportResult.getUserId());
            return (user != null) ? user.getName() : ("[" + reportResult.getUserId() + "]"); //$NON-NLS-1$ //$NON-NLS-2$
			case ReportExecutionForm.RESULT_STATUS:
			   return Messages.get().ReportResultLabelProvider_Success; // TODO: get actual job status
		}
		return "<INTERNAL ERROR>"; //$NON-NLS-1$
	}
}
