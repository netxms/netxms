/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.actions;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ASN;
import org.netxms.client.constants.DataCollectionObjectStatus;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.CreateSnmpDciDialog;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.snmp.shared.MibCache;
import org.xnap.commons.i18n.I18n;

/**
 * "Create DCI from SNMP walk result" action
 */
public class CreateSnmpDci extends Action
{
   private I18n i18n = LocalizationHelper.getI18n(CreateSnmpDci.class);
   
	private Shell shell;
	private List<SnmpValue> objects = new ArrayList<SnmpValue>();
	private ObjectView view;

	public CreateSnmpDci(ObjectView view)
	{
	   super(LocalizationHelper.getI18n(CreateSnmpDci.class).tr("Create data collection tiem..."));
	   this.view = view;
	}

	/**
	 * @see org.eclipse.jface.action.Action#run()
	 */
	@Override
	public void run()
	{
		if (objects.size() == 0)
			return;
		
		MibObject mibObject = MibCache.findObject(objects.get(0).getName(), false);
		String description = (mibObject != null) ? mibObject.getName() : objects.get(0).getName();
		if (objects.size() > 1)
			description += " @@instance@@"; //$NON-NLS-1$
		
		final CreateSnmpDciDialog dlg = new CreateSnmpDciDialog(shell, description);
		if (dlg.open() != Window.OK)
			return;
		
		final NXCSession session = Registry.getSession();
		final List<SnmpValue> values = new ArrayList<SnmpValue>(objects);
		
		// Get set of nodes
		final Set<AbstractNode> nodes = new HashSet<AbstractNode>();
		for(SnmpValue v : values)
		{
			AbstractNode node = (AbstractNode)session.findObjectById(v.getNodeId(), AbstractNode.class);
			if (node != null)
			{
				nodes.add(node);
			}
		}
      
      new Job(i18n.tr("Creating new SNMP DCI..."), view) {
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create DCI");
         }

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            monitor.beginTask(i18n.tr("Creating data collection items"), values.size());
            for(SnmpValue v : values)
            {
               final String description = dlg.getDescription().replaceAll("@@instance@@", Long.toString(v.getObjectId().getIdFromPos(v.getObjectId().getLength() - 1))); //$NON-NLS-1$
               createDci(session, v, description, dlg.getPollingInterval(), 
                     dlg.getRetentionTime(), dlg.getDeltaCalculation());
               monitor.worked(1);
            }
            monitor.done();
         }
      }.start();
	}

	/**
	 * @param session
	 * @param value
	 * @param description
	 * @param pollingInterval
	 * @param retentionTime
	 * @param deltaCalculation
	 * @param lockRequired
	 * @throws Exception
	 */
	private static void createDci(NXCSession session, SnmpValue value, String description, int pollingInterval,
			int retentionTime, int deltaCalculation) throws Exception
	{
		AbstractNode node = (AbstractNode)session.findObjectById(value.getNodeId(), AbstractNode.class);
		if (node == null)
			throw new NXCException(RCC.INTERNAL_ERROR);		
		
		DataCollectionConfiguration dcc;
		dcc = session.openDataCollectionConfiguration(node.getObjectId());

		final DataCollectionItem dci = new DataCollectionItem(dcc, 0);
      dci.setPollingScheduleType(DataCollectionObject.POLLING_SCHEDULE_CUSTOM);
      dci.setPollingInterval(Integer.toString(pollingInterval));
      dci.setRetentionType(DataCollectionObject.RETENTION_CUSTOM);
      dci.setRetentionTime(Integer.toString(retentionTime));
      dci.setOrigin(DataOrigin.SNMP);
		dci.setDataType(dciTypeFromAsnType(value.getType()));
      dci.setStatus(DataCollectionObjectStatus.ACTIVE);
		dci.setDescription(description);
		dci.setDeltaCalculation(deltaCalculation);
		dci.setName(value.getName());

		dcc.modifyObject(dci);
		dcc.close();
	}
	
	/**
	 * Convert ASN.1 identifier type to DCI data type
	 * 
	 * @param type ASN.1 identifier type
	 * @return DCI data type
	 */
	public static DataType dciTypeFromAsnType(int type)
	{
		switch(type)
		{
			case ASN.COUNTER32:
            return DataType.COUNTER32;
         case ASN.COUNTER64:
            return DataType.COUNTER64;
			case ASN.GAUGE32:
         case ASN.TIMETICKS:
			case ASN.UINTEGER32:
				return DataType.UINT32;
			case ASN.INTEGER:
				return DataType.INT32;
			default:
				return DataType.STRING;
		}
	}

	/**
	 * Selection change 
	 * 
	 * @param selection new selection
	 */
	public void selectionChanged(ISelection selection)
	{
		objects.clear();
		if ((selection instanceof IStructuredSelection) &&
			 (((IStructuredSelection)selection).size() > 0))
		{
			for(Object o : ((IStructuredSelection)selection).toList())
			{
				if (o instanceof SnmpValue)
					objects.add((SnmpValue)o);
			}
			
			this.setEnabled(objects.size() > 0);
		}
		else
		{
			this.setEnabled(false);
		}
	}
}
