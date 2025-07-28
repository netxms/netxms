/**
 * NetXMS - open source network management system
 * Copyright (C) 2025 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.dialogs.helpers;

import org.netxms.client.datacollection.BulkDciUpdateElement;

/**
 * Information about changed field for bulk threshold DCI update
 */
public class BulkDciThresholdUpdateElement extends BulkDciUpdateElement
{   
   /**
    * Create new update element for given field
    *
    * @param fieldId field ID
    */
   public BulkDciThresholdUpdateElement(long fieldId, Long value)
   {
      super(fieldId);
      this.value = value;
   }
   
}
