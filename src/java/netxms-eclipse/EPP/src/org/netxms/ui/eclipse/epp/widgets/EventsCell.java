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

import java.util.List;

import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.epp.Activator;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * Cell containing events
 *
 */
public class EventsCell extends Cell
{
	private EventProcessingPolicyRule eppRule;
	private TableViewer viewer;
	private TableColumn column;
	private CLabel labelAny;
	private NXCSession session;
	
	public EventsCell(Rule rule, Object data)
	{
		super(rule, data);
		eppRule = (EventProcessingPolicyRule)data;
		session = NXMCSharedData.getInstance().getSession();
		
		viewer = new TableViewer(this, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new WorkbenchLabelProvider());
		
		column = new TableColumn(viewer.getTable(), SWT.LEFT);
		column.setResizable(false);

		labelAny = new CLabel(this, SWT.LEFT);
		labelAny.setText("Any");
		labelAny.setImage(Activator.getImageDescriptor("icons/any.gif").createImage());
		labelAny.setBackground(PolicyEditor.COLOR_BACKGROUND);
		
		setLayout(null);
		addControlListener(new ControlAdapter() {
			/* (non-Javadoc)
			 * @see org.eclipse.swt.events.ControlAdapter#controlResized(org.eclipse.swt.events.ControlEvent)
			 */
			@Override
			public void controlResized(ControlEvent e)
			{
				final Point size = getSize();
				column.setWidth(size.x);
				viewer.getTable().setSize(size);
				labelAny.setSize(size);
			}
		});
		
		List<Long> eventCodes = eppRule.getEvents();

		viewer.setInput(session.findMultipleEventTemplates(eventCodes.toArray(new Long[eventCodes.size()])).toArray());
		if (eventCodes.size() > 0)
		{
			labelAny.setVisible(false);
		}
		else
		{
			viewer.getTable().setVisible(false);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		return (eppRule.getEvents().size() > 0) ? viewer.getTable().computeSize(wHint, hHint, changed) : labelAny.computeSize(wHint, hHint, changed);
	}
}
