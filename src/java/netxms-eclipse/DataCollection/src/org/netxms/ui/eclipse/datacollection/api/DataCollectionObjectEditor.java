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
package org.netxms.ui.eclipse.datacollection.api;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.jobs.ConsoleJob;

/**
 * Helper class for handling modifications in data collection objects
 */
public class DataCollectionObjectEditor
{
	DataCollectionObject object;
	private Runnable timer;
	private Set<DataCollectionObjectListener> listeners = new HashSet<DataCollectionObjectListener>(); 

	/**
	 * @param object
	 */
	public DataCollectionObjectEditor(DataCollectionObject object)
	{
		this.object = object;
		timer = new Runnable() {
			@Override
			public void run()
			{
				doObjectModification();
			}
		};
	}
	
	/**
	 * Do object modification on server
	 */
	private void doObjectModification()
	{
		new ConsoleJob(Messages.DataCollectionObjectEditor_JobName, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				synchronized(DataCollectionObjectEditor.this)
				{
					object.getOwner().modifyObject(object);
				}
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						Object data = object.getOwner().getUserData();
						if ((data != null) && (data instanceof TableViewer))
							((TableViewer)data).update(object, null);
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.DataCollectionObjectEditor_JobError;
			}
		}.start();
	}
	
	/**
	 * Schedule object modification
	 */
	public void modify()
	{
		Display.getCurrent().timerExec(-1, timer);
		Display.getCurrent().timerExec(200, timer);
	}
	
	/**
	 * @param listener
	 */
	public void addListener(DataCollectionObjectListener listener)
	{
		listeners.add(listener);
	}
	
	/**
	 * @param listener
	 */
	public void removeListener(DataCollectionObjectListener listener)
	{
		listeners.remove(listener);
	}
	
	/**
	 * @param origin
	 * @param name
	 * @param description
	 * @param dataType
	 */
	public void fireOnSelectItemListeners(int origin, String name, String description, int dataType)
	{
		for(DataCollectionObjectListener l : listeners)
			l.onSelectItem(origin, name, description, dataType);
	}

	/**
	 * @param origin
	 * @param name
	 * @param description
	 */
	public void fireOnSelectTableListeners(int origin, String name, String description)
	{
		for(DataCollectionObjectListener l : listeners)
			l.onSelectTable(origin, name, description);
	}

	/**
	 * @return the object
	 */
	public DataCollectionObject getObject()
	{
		return object;
	}

	/**
	 * @return the object
	 */
	public DataCollectionItem getObjectAsItem()
	{
		return (DataCollectionItem)object;
	}

	/**
	 * @return the object
	 */
	public DataCollectionTable getObjectAsTable()
	{
		return (DataCollectionTable)object;
	}
}
