/**
 * 
 */
package org.netxms.ui.eclipse.eventmanager;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.eventmanager.views.EventConfigurator;

/**
 * @author Victor
 *
 */
public class EventTemplateLabelProvider extends WorkbenchLabelProvider implements ITableLabelProvider
{
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return (columnIndex == 0) ? getImage(element) : null;
	}

	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case EventConfigurator.COLUMN_CODE:
				return Long.toString(((EventTemplate)element).getCode());
			case EventConfigurator.COLUMN_NAME:
				return getText(element);
			case EventConfigurator.COLUMN_SEVERITY:
				return StatusDisplayInfo.getStatusText(((EventTemplate)element).getSeverity());
			case EventConfigurator.COLUMN_FLAGS:
				return ((((EventTemplate)element).getFlags() & EventTemplate.FLAG_WRITE_TO_LOG) != 0) ? "L" : "-";
			case EventConfigurator.COLUMN_MESSAGE:
				return ((EventTemplate)element).getMessage();
			case EventConfigurator.COLUMN_DESCRIPTION:
				return ((EventTemplate)element).getDescription();
		}
		return null;
	}
}
