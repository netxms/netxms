/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets.helpers;

import org.netxms.client.objects.configs.RoomPassiveElement;

/**
 * Listener for floor plan editing events
 */
public interface FloorPlanEditListener
{
   /**
    * Called when floor plan model was changed by user interaction.
    */
   public void modelChanged();

   /**
    * Called when user requested editing of passive element (double click in edit mode).
    *
    * @param element element to edit
    */
   public void editElementRequested(RoomPassiveElement element);

   /**
    * Called when user selected two calibration points on the backdrop.
    *
    * @param imagePixelDistance distance between the points in backdrop image pixels
    */
   public void calibrationPointsSelected(double imagePixelDistance);
}
