/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.config;

import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * Layout configuration for dashboard element
 */
@Root(name="layout", strict=false)
public class DashboardElementLayout
{
	@Element(required=false)
	public int horizontalSpan = 1;
	
	@Element(required=false)
	public int verticalSpan = 1;
	
	@Element(required=false)
	public boolean grabVerticalSpace = true;

	@Element(required=false)
	public int heightHint = -1;

   @Element(required = false)
   public boolean showInNarrowScreenMode = true;

   @Element(required = false)
   public int narrowScreenOrder = 255;

   @Element(required = false)
   public int narrowScreenHeightHint = -1;
}
