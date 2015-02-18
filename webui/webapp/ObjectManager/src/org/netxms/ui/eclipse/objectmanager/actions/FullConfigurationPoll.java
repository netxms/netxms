/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.actions;

import org.netxms.client.constants.NodePollType;

/**
 * Start configuration poll
 */
public class FullConfigurationPoll extends AbstractNodePoll
{
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectmanager.actions.AbstractNodePoll#getPollType()
	 */
	@Override
	protected NodePollType getPollType()
	{
		return NodePollType.CONFIGURATION_FULL;
	}

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectmanager.actions.AbstractNodePoll#getConfirmation()
    */
   @Override
   protected String getConfirmation()
   {
      return "Full configuration poll will reset node capabilities and can possibly change container and template binding as well as delete DCIs created by instance discovery. Continue?";
   }
}
