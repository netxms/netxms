/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.epp.widgets.helpers.ImageFactory;

/**
 * Cell for severity filter
 *
 */
public class SeverityCell extends ListCell
{
	private EventProcessingPolicyRule eppRule;
	private Action[] actionSeverity = new Action[5];
	
	public SeverityCell(Rule rule, Object data)
	{
		super(rule, data);
		eppRule = (EventProcessingPolicyRule)data;
		makeActions();	
		updateSeverityList();
	}

	/**
	 * Update list of matched severity
	 */
	private void updateSeverityList()
	{
		clear();
		
		int flags = eppRule.getFlags();
		if ((flags & EventProcessingPolicyRule.SEVERITY_ANY) == EventProcessingPolicyRule.SEVERITY_ANY)
		{
			addEntry("Any", ImageFactory.getImage("icons/any.gif"));
		}
		else
		{
			int mask = EventProcessingPolicyRule.SEVERITY_CRITICAL;
			for(int i = 4; i >= 0; i--)
			{
				if ((flags & mask) == mask)
				{
					addEntry(StatusDisplayInfo.getStatusText(i), ImageFactory.getImage(StatusDisplayInfo.getStatusImageDescriptor(i)));
				}
				mask >>= 1;
			}
		}
	}

	/**
	 * Make actions
	 */
	private void makeActions()
	{
		int flag = EventProcessingPolicyRule.SEVERITY_NORMAL;
		for(int i = 0; i < 5; i++, flag <<= 1)
		{
			final Integer severityFlag = new Integer(flag);
			actionSeverity[i] = new Action() {
				/* (non-Javadoc)
				 * @see org.eclipse.jface.action.Action#run()
				 */
				@Override
				public void run()
				{
					int flags = eppRule.getFlags();
					if ((flags & severityFlag) != 0)
					{
						flags &= ~severityFlag;
					}
					else
					{
						flags |= severityFlag;
					}
					eppRule.setFlags(flags);
					setChecked((flags & severityFlag) != 0);
					updateSeverityList();
					contentChanged();
				}
			};
			actionSeverity[i].setText(StatusDisplayInfo.getStatusText(i));
			actionSeverity[i].setChecked((eppRule.getFlags() & flag) != 0);
		}
	}
	
	/**
	 * Fill cell's context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		for(int i = 0; i < 5; i++)
			mgr.add(actionSeverity[i]);
	}
}
