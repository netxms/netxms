/**
 * 
 */
package org.netxms.ui.eclipse.topology.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.topology.Port;
import org.netxms.client.topology.VlanInfo;
import org.netxms.ui.eclipse.topology.views.VlanView;

/**
 * Label provider for VLAN list
 *
 */
public class VlanLabelProvider extends LabelProvider implements ITableLabelProvider
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		VlanInfo vlan = (VlanInfo)element;
		switch(columnIndex)
		{
			case VlanView.COLUMN_VLAN_ID:
				return Integer.toString(vlan.getVlanId());
			case VlanView.COLUMN_NAME:
				return vlan.getName();
			case VlanView.COLUMN_PORTS:
				return buildPortList(vlan);
			default:
				break;
		}
		return null;
	}
	
	/**
	 * Build list of ports in VLAN
	 * 
	 * @param vlan VLAN object
	 * @return port list
	 */
	private String buildPortList(VlanInfo vlan)
	{
		Port[] ports = vlan.getPorts();
		if (ports.length == 0)
			return "";
		
		int slot = ports[0].getSlot();
		int lastPort = ports[0].getPort();
		int firstPort = lastPort;
		
		StringBuilder sb = new StringBuilder();
		sb.append(slot);
		sb.append('/');
		sb.append(firstPort);
		
		int i;
		for(i = 1; i < ports.length; i++)
		{
			if ((ports[i].getSlot() == slot) && (ports[i].getPort() == lastPort + 1))
			{
				lastPort++;
				continue;
			}
			
			// If previous series was not single port, add ending port
			if (ports[i - 1].getPort() != firstPort)
			{
				if (lastPort - firstPort > 1)
					sb.append('-');
				else
					sb.append(',');
				sb.append(slot);
				sb.append('/');
				sb.append(lastPort);
			}
			
			slot = ports[i].getSlot();
			lastPort = ports[i].getPort();
			firstPort = lastPort;
			
			sb.append(',');
			sb.append(slot);
			sb.append('/');
			sb.append(lastPort);
		}

		// If previous series was not single port, add ending port
		if (ports[i - 1].getPort() != firstPort)
		{
			if (lastPort - firstPort > 1)
				sb.append('-');
			else
				sb.append(',');
			sb.append(slot);
			sb.append('/');
			sb.append(lastPort);
		}
		
		return sb.toString();
	}
}
