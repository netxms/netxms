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
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * Cell with source objects list
 *
 */
public class ObjectsCell extends Cell
{
	private EventProcessingPolicyRule eppRule;
	private TableViewer viewer;
	private TableColumn column;
	private NXCSession session;

	public ObjectsCell(Rule rule, Object data)
	{
		super(rule, data);
		eppRule = (EventProcessingPolicyRule)data;
		session = NXMCSharedData.getInstance().getSession();
		
		viewer = new TableViewer(this, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new WorkbenchLabelProvider());
		
		column = new TableColumn(viewer.getTable(), SWT.LEFT);
		column.setResizable(false);

		addControlListener(new ControlAdapter() {
			/* (non-Javadoc)
			 * @see org.eclipse.swt.events.ControlAdapter#controlResized(org.eclipse.swt.events.ControlEvent)
			 */
			@Override
			public void controlResized(ControlEvent e)
			{
				column.setWidth(getSize().x);
			}
		});
		
		List<Long> objectIds = eppRule.getSources();
		viewer.setInput(session.findMultipleObjects(objectIds.toArray(new Long[objectIds.size()])));
	}
}
