/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.webui.core;

import org.eclipse.ui.internal.dialogs.PropertyPageNode;
import org.eclipse.ui.model.ContributionComparator;

/**
 * Extended contribution comparator. Takes priority encoded in
 * contribution id after # symbol into account. Contribution with lowest
 * priority number wins.
 *
 */
@SuppressWarnings("restriction")
public class ExtendedContributionComparator extends ContributionComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.ui.model.ContributionComparator#compare(java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Object o1, Object o2)
	{
		PropertyPageNode node1 = null, node2 = null;
		
		if (o1 instanceof PropertyPageNode)
			node1 = (PropertyPageNode)o1;
		
		if (o2 instanceof PropertyPageNode)
			node2 = (PropertyPageNode)o2;
		
		if ((node1 == null) || (node2 == null))
			return super.compare(o1, o2);
		
		int pri1 = getPriority(node1);
		int pri2 = getPriority(node2);
		
		// if both have no priority or equal priority, compare as usual
		if (pri1 == pri2)
			return super.compare(o1, o2);
		
		// node with priority wins
		if (pri1 == -1)
			return 1;

		if (pri2 == -1)
			return -1;
		
		return pri1 - pri2;
	}

	/**
	 * Extract page priority from page id. Expects page id to be in format id#priority. If priority is not given,
	 * -1 is returned.
	 * 
	 * @param node contribution item
	 * @return extracted priority
	 */
	private int getPriority(PropertyPageNode node)
	{
		int idx = node.getId().indexOf('#');
		if (idx == -1)
			return -1;
		try
		{
			return Integer.parseInt(node.getId().substring(idx + 1));
		}
		catch(Exception e)
		{
			return -1;
		}
	}
}
