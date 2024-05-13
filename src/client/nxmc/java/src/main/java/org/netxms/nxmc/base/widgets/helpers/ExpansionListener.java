/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets.helpers;

/**
 * Classes that implement this interface will be notified after the expandable control's expansion state changes.
 */
@FunctionalInterface
public interface ExpansionListener
{
   /**
    * Notifies the listener after the expandable control has changed its expansion state. The state provided by the event is the new
    * state.
    *
    * @param e the expansion event
    */
   void expansionStateChanged(ExpansionEvent e);
}
