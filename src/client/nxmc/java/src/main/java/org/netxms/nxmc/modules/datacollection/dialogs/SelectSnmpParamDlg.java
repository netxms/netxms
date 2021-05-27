/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import org.eclipse.swt.widgets.Shell;
import org.netxms.client.constants.DataType;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.nxmc.modules.snmp.dialogs.MibSelectionDialog;

/**
 * SNMP OID selection dialog
 */
public class SelectSnmpParamDlg extends MibSelectionDialog implements IParameterSelectionDialog
{
	/**
	 * @param parentShell
	 * @param currentOid
	 * @param nodeId
	 */
	public SelectSnmpParamDlg(Shell parentShell, SnmpObjectId currentOid, long nodeId)
	{
		super(parentShell, currentOid, nodeId);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDataType()
	 */
	@Override
	public DataType getParameterDataType()
	{
		switch(getSelectedObject().getType())
		{
	      case MibObject.MIB_TYPE_INTEGER:
	      case MibObject.MIB_TYPE_INTEGER32:
				return DataType.INT32;
			case MibObject.MIB_TYPE_COUNTER:
			case MibObject.MIB_TYPE_COUNTER32:
            return DataType.COUNTER32;
			case MibObject.MIB_TYPE_GAUGE:
			case MibObject.MIB_TYPE_GAUGE32:
	      case MibObject.MIB_TYPE_TIMETICKS:
	      case MibObject.MIB_TYPE_UINTEGER:
	      case MibObject.MIB_TYPE_UNSIGNED32:
				return DataType.UINT32;
			case MibObject.MIB_TYPE_COUNTER64:
            return DataType.COUNTER64;
		}
      return DataType.STRING;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDescription()
	 */
	@Override
	public String getParameterDescription()
	{
		return getSelectedObject().getName();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterName()
	 */
	@Override
	public String getParameterName()
	{
		return getSelectedObjectId().toString();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getInstanceColumn()
	 */
	@Override
	public String getInstanceColumn()
	{
		return ""; //$NON-NLS-1$
	}
}
