/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.base.actions;

import org.eclipse.jface.action.Action;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;

/**
 * Refresh action - provides correct icon and text
 */
public class RefreshAction extends Action
{
   public static final String ID = "Global.Refresh";

	/**
	 * Create default refresh action
	 */
	public RefreshAction()
	{
      super(LocalizationHelper.getI18n(RefreshAction.class).tr("&Refresh"), SharedIcons.REFRESH);
      setId(ID);
	}
}
