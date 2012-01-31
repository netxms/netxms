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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * "Communications" property page for zone objects
 */
public class ZoneCommunications extends PropertyPage
{
	private static final long serialVersionUID = 1L;

	private Zone zone;
	private ObjectSelector agentProxy;
	private ObjectSelector snmpProxy;
	private ObjectSelector icmpProxy;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		zone = (Zone)getElement().getAdapter(Zone.class);

		Composite dialogArea = new Composite(parent, SWT.NONE);
		GridLayout dialogLayout = new GridLayout();
		dialogLayout.marginWidth = 0;
		dialogLayout.marginHeight = 0;
		dialogArea.setLayout(dialogLayout);

		agentProxy = new ObjectSelector(dialogArea, SWT.NONE);
		agentProxy.setLabel("Default agent proxy");
		agentProxy.setObjectId(zone.getAgentProxy());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		agentProxy.setLayoutData(gd);
		
		snmpProxy = new ObjectSelector(dialogArea, SWT.NONE);
		snmpProxy.setLabel("Default SNMP proxy");
		snmpProxy.setObjectId(zone.getSnmpProxy());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		snmpProxy.setLayoutData(gd);
		
		icmpProxy = new ObjectSelector(dialogArea, SWT.NONE);
		icmpProxy.setLabel("Default ICMP proxy");
		icmpProxy.setObjectId(zone.getIcmpProxy());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		icmpProxy.setLayoutData(gd);
		
		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);
		
		final NXCObjectModificationData md = new NXCObjectModificationData(zone.getObjectId());
		md.setAgentProxy(agentProxy.getObjectId());
		md.setSnmpProxy(snmpProxy.getObjectId());
		md.setIcmpProxy(icmpProxy.getObjectId());
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Update communication settings for zone " + zone.getObjectName(), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update communication settings";
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							ZoneCommunications.this.setValid(true);
						}
					});
				}
			}
		}.start();
		return true;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		return applyChanges(false);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		
		agentProxy.setObjectId(0);
		snmpProxy.setObjectId(0);
		icmpProxy.setObjectId(0);
	}
}
