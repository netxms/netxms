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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.ui.eclipse.snmp.dialogs.MibSelectionDialog;

/**
 * SNMP OID selection dialog
 *
 */
public class SelectSnmpParamDlg extends MibSelectionDialog implements IParameterSelectionDialog
{
	private static final long serialVersionUID = 1L;

	/**
	 * @param parentShell
	 */
	public SelectSnmpParamDlg(Shell parentShell, SnmpObjectId currentOid)
	{
		super(parentShell, currentOid);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDataType()
	 */
	@Override
	public int getParameterDataType()
	{
		switch(getSelectedObject().getType())
		{
	      case MibObject.MIB_TYPE_INTEGER:
	      case MibObject.MIB_TYPE_INTEGER32:
				return DataCollectionItem.DT_INT;
			case MibObject.MIB_TYPE_COUNTER:
			case MibObject.MIB_TYPE_COUNTER32:
			case MibObject.MIB_TYPE_GAUGE:
			case MibObject.MIB_TYPE_GAUGE32:
	      case MibObject.MIB_TYPE_TIMETICKS:
	      case MibObject.MIB_TYPE_UINTEGER:
	      case MibObject.MIB_TYPE_UNSIGNED32:
				return DataCollectionItem.DT_UINT;
			case MibObject.MIB_TYPE_COUNTER64:
				return DataCollectionItem.DT_UINT64;
		}
		return DataCollectionItem.DT_STRING;
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
		return getSelectedObject().getObjectId().toString();
	}
}
