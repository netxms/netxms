/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.views.DataCollectionView;
import org.xnap.commons.i18n.I18n;

/**
 * Helper class for handling modifications in data collection objects
 */
public class DataCollectionObjectEditor
{
   private final I18n i18n = LocalizationHelper.getI18n(DataCollectionObjectEditor.class);

	private DataCollectionObject object;
	private long sourceNode;
	private Runnable timer;
	private Set<DataCollectionObjectListener> listeners = new HashSet<DataCollectionObjectListener>(); 
   private TableColumnEnumerator tableColumnEnumerator;

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
      new Job(i18n.tr("Modify data collection object"), null) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final boolean isNewObj = object.isNewItem();
				synchronized(DataCollectionObjectEditor.this)
				{
				   long itemId = object.getOwner().modifyObject(object);
				   if(object.isNewItem())
				      object.setId(itemId);
				}
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						Object data = object.getOwner().getUserData();
						if ((data != null) && (data instanceof DataCollectionView))
						{
	                  if (isNewObj)
	                  {
	                     ((DataCollectionView)data).setInput(object.getOwner().getItems());
	                  }
							((DataCollectionView)data).update(object);
						}
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
			   Object data = object.getOwner().getUserData();
            if ((data != null) && (data instanceof DataCollectionView))
            {
               ((DataCollectionView)data).refresh();
            }
            return i18n.tr("Cannot modify data collection object");
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
   public void fireOnSelectItemListeners(DataOrigin origin, String name, String description, DataType dataType)
	{
		for(DataCollectionObjectListener l : listeners)
			l.onSelectItem(origin, name, description, dataType);
	}

	/**
	 * @param origin
	 * @param name
	 * @param description
	 */
   public void fireOnSelectTableListeners(DataOrigin origin, String name, String description)
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

   /**
    * Get table column enumerator currently associated with this editor.
    *
    * @return table column enumerator currently associated with this editor or null
    */
   public TableColumnEnumerator getTableColumnEnumerator()
   {
      return tableColumnEnumerator;
   }

   /**
    * Set new table column enumerator for this editor.
    *
    * @param enumerator new table column enumerator
    */
   public void setTableColumnEnumerator(TableColumnEnumerator enumerator)
   {
      this.tableColumnEnumerator = enumerator;
   }

   /**
    * Sets temporary source node ID
    */
   public void setSourceNode(long nodeId)
   {
      sourceNode = nodeId;
   }
   
   /**
    * @return temporary source node ID
    */
   public long getSourceNode()
   {
      return sourceNode;
   }
   
   /**
    * Create DCI modification warning message. Returns message ready to display or null if message is not needed.
    * 
    * @param dco data collection object
    * @return warning message or null
    */
   public static String createModificationWarningMessage(DataCollectionObject dco)
   {
      I18n i18n = LocalizationHelper.getI18n(DataCollectionObjectEditor.class);
      String message = null;
      if (dco.getTemplateId() == dco.getNodeId())
      {
         message = i18n.tr("This DCI was added by instance discovery\nAll local changes can be overwritten at any moment");
      }
      else if (dco.getTemplateId() != 0)
      {
         AbstractObject object = Registry.getSession().findObjectById(dco.getTemplateId());
         if (object != null)
         {
            message = String.format(
                  (object.getObjectClass() == AbstractObject.OBJECT_CLUSTER) ?
                        i18n.tr("This DCI was added by cluster \"%s\"\nAll local changes can be overwritten at any moment") :
                           i18n.tr("This DCI was added by template \"%s\"\nAll local changes can be overwritten at any moment"),
                  object.getObjectName());
         }
         else
         {
            message = String.format(i18n.tr("This DCI was added by unknown object with ID %d\nAll local changes can be overwritten at any moment"), dco.getTemplateId());
         }
      }
      return message;
   }
}
