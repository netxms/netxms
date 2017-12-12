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
package org.netxms.ui.eclipse.objectview.widgets;

import org.eclipse.jface.action.Action;

/**
 * Widget showing "heat" map of nodes under given root object
 */
public interface ObjectStatusMapInterface 
{
	/**
	 * @param objectId
	 */
	public void setRootObject(long objectId);
	
	/**
	 * Refresh form
	 */
	public void refresh();

	/**
	 * @return
	 */
	public int getSeverityFilter();

	/**
	 * @param severityFilter
	 */
	public void setSeverityFilter(int severityFilter);
	
	/**
	 * @param listener
	 */
	public void addRefreshListener(Runnable listener);
	
	/**
	 * @param listener
	 */
	public void removeRefreshListener(Runnable listener);

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable);

   /**
    * @return the filterEnabled
    */
   public boolean isFilterEnabled();
   
   /**
    * Set action to be executed when user press "Close" button in object filter.
    * Default implementation will hide filter area without notifying parent.
    * 
    * @param action
    */
   public void setFilterCloseAction(Action action);
   
   /**
    * Set filter text
    * 
    * @param text New filter text
    */
   public void setFilterText(final String text);

   /**
    * Get filter text
    * 
    * @return Current filter text
    */
   public String getFilterText();
}
