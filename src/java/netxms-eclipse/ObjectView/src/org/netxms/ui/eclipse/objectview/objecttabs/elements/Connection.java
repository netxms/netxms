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
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * "Connection" element - shows peer information for interface
 */
public class Connection extends OverviewPageElement
{
	private NXCSession session;
	private CLabel nodeLabel;
	private CLabel interfaceLabel;
   private CLabel protocolLabel;
	private WorkbenchLabelProvider labelProvider;
	
	/**
	 * @param parent
	 */
	public Connection(Composite parent, OverviewPageElement anchor)
	{
		super(parent, anchor);
		session = (NXCSession)ConsoleSharedData.getSession();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.DashboardElement#createClientArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createClientArea(Composite parent)
	{
		labelProvider = new WorkbenchLabelProvider();
		parent.addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				labelProvider.dispose();
			}
		});
		
		Composite area = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		area.setLayout(layout);
		area.setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_BACKGROUND, parent.getDisplay()));
		
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
		
      protocolLabel = new CLabel(area, SWT.NONE);
      protocolLabel.setBackground(area.getBackground());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalIndent = 15;
      protocolLabel.setLayoutData(gd);
      
		return area;
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
	 */
	@Override
	protected String getTitle()
	{
		return Messages.get().Connection_Title;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
	 */
	@Override
	protected void onObjectChange()
	{
		if ((getObject() == null) || !(getObject() instanceof Interface))
			return;
		
		Interface iface = (Interface)getObject();
		long peerNodeId = iface.getPeerNodeId();
		if (peerNodeId != 0)
		{
			AbstractObject node = session.findObjectById(peerNodeId);
			nodeLabel.setText((node != null) ? node.getObjectName() : "<" + peerNodeId + ">"); //$NON-NLS-1$ //$NON-NLS-2$
			nodeLabel.setImage(labelProvider.getImage(node));
		}
		else
		{
			nodeLabel.setText(Messages.get().Connection_NA);
		}
		
		long peerInterfaceId = iface.getPeerInterfaceId();
		if (peerInterfaceId != 0)
		{
			AbstractObject peerIface = session.findObjectById(peerInterfaceId);
			interfaceLabel.setText((peerIface != null) ? peerIface.getObjectName() : "<" + peerInterfaceId + ">"); //$NON-NLS-1$ //$NON-NLS-2$
			interfaceLabel.setImage(labelProvider.getImage(peerIface));
			protocolLabel.setText(iface.getPeerDiscoveryProtocol().toString());
		}
		else
		{
			interfaceLabel.setText(Messages.get().Connection_NA);
			protocolLabel.setText(""); //$NON-NLS-1$
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean isApplicableForObject(AbstractObject object)
	{
		return (object instanceof Interface) && (((Interface)object).getPeerNodeId() != 0);
	}
}
