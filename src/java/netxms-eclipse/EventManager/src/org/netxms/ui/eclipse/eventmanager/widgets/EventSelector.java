/**
 * 
 */
package org.netxms.ui.eclipse.eventmanager.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.eventmanager.dialogs.EventSelectionDialog;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * @author Victor
 *
 */
public class EventSelector extends AbstractSelector
{
	private long eventCode = 0;
	
	/**
	 * @param parent
	 * @param style
	 */
	public EventSelector(Composite parent, int style)
	{
		super(parent, style);
		setText("<none>");
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		EventSelectionDialog dlg = new EventSelectionDialog(getShell());
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
			EventTemplate[] events = dlg.getSelectedEvents();
			if (events.length > 0)
			{
				eventCode = events[0].getCode();
				setText(events[0].getName());
			}
			else
			{
				eventCode = 0;
				setText("<none>");
			}
		}
	}

	/**
	 * Get code of selected event
	 * 
	 * @return Selected event's code
	 */
	public long getEventCode()
	{
		return eventCode;
	}

	/**
	 * Set event code
	 * @param eventCode
	 */
	public void setEventCode(long eventCode)
	{
		this.eventCode = eventCode;
		if (eventCode != 0)
		{
			EventTemplate event = NXMCSharedData.getInstance().getSession().findEventTemplateByCode(eventCode);
			if (event != null)
			{
				setText(event.getName());
			}
			else
			{
				setText("<unknown>");
			}
		}
		else
		{
			setText("<none>");
		}
	}
}
