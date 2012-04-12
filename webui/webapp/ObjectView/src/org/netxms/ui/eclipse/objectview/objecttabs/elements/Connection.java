/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedColors;

/**
 * "Connection" element - shows peer information for interface
 */
public class Connection extends OverviewPageElement
{
	private static final long serialVersionUID = 1L;

	private NXCSession session;
	private CLabel nodeLabel;
	private CLabel interfaceLabel;
	private WorkbenchLabelProvider labelProvider;
	
	/**
	 * @param parent
	 * @param object
	 */
	public Connection(Composite parent, GenericObject object)
	{
		super(parent, object);
		session = (NXCSession)ConsoleSharedData.getSession();
		labelProvider = new WorkbenchLabelProvider();
		addDisposeListener(new DisposeListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				labelProvider.dispose();
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.DashboardElement#createClientArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createClientArea(Composite parent)
	{
		Composite area = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		area.setLayout(layout);
		area.setBackground(SharedColors.WHITE);
		
		nodeLabel = new CLabel(area, SWT.NONE);
		nodeLabel.setBackground(area.getBackground());
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		nodeLabel.setLayoutData(gd);
		
		interfaceLabel = new CLabel(area, SWT.NONE);
		interfaceLabel.setBackground(area.getBackground());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalIndent = 15;
		interfaceLabel.setLayoutData(gd);
		
		return area;
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
	 */
	@Override
	protected String getTitle()
	{
		return "Connection";
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
	 */
	@Override
	void onObjectChange()
	{
		if ((getObject() == null) || !(getObject() instanceof Interface))
			return;
		
		Interface iface = (Interface)getObject();
		long peerNodeId = iface.getPeerNodeId();
		if (peerNodeId != 0)
		{
			GenericObject node = session.findObjectById(peerNodeId);
			nodeLabel.setText((node != null) ? node.getObjectName() : "<" + peerNodeId + ">");
			nodeLabel.setImage(labelProvider.getImage(node));
		}
		else
		{
			nodeLabel.setText("N/A");
		}
		
		long peerInterfaceId = iface.getPeerInterfaceId();
		if (peerInterfaceId != 0)
		{
			GenericObject peerIface = session.findObjectById(peerInterfaceId);
			interfaceLabel.setText((peerIface != null) ? peerIface.getObjectName() : "<" + peerInterfaceId + ">");
			interfaceLabel.setImage(labelProvider.getImage(peerIface));
		}
		else
		{
			interfaceLabel.setText("N/A");
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public boolean isApplicableForObject(GenericObject object)
	{
		return (object instanceof Interface) && (((Interface)object).getPeerNodeId() != 0);
	}
}
