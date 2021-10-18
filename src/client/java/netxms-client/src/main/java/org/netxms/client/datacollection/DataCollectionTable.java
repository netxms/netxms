/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.client.datacollection;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Tabular data collection object
 */
public class DataCollectionTable extends DataCollectionObject
{
   // Instance discovery methods
   public static final int IDM_NONE = 0;
   public static final int IDM_AGENT_LIST = 1;
   public static final int IDM_AGENT_TABLE = 2;
   public static final int IDM_SNMP_WALK_VALUES = 3;
   public static final int IDM_SNMP_WALK_OIDS = 4;
   public static final int IDM_SCRIPT = 5;
   
	private String instanceColumn;
	private List<ColumnDefinition> columns;
	private List<TableThreshold> thresholds;

	/**
    * Create data collection object from NXCP message.
    *
    * @param owner The owner object
    * @param msg The NXCPMessage
    */
	public DataCollectionTable(DataCollectionConfiguration owner, NXCPMessage msg)
	{
		super(owner, msg);
		instanceColumn = msg.getFieldAsString(NXCPCodes.VID_INSTANCE_COLUMN);
		
		int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_COLUMNS);
		columns = new ArrayList<ColumnDefinition>(count);
		long fieldId = NXCPCodes.VID_DCI_COLUMN_BASE;
		for(int i = 0; i < count; i++)
		{
			columns.add(new ColumnDefinition(msg, fieldId));
			fieldId += 10;
		}

		count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_THRESHOLDS);
		thresholds = new ArrayList<TableThreshold>(count);
		fieldId = NXCPCodes.VID_DCI_THRESHOLD_BASE;
		for(int i = 0; i < count; i++)
		{
			final TableThreshold t = new TableThreshold(msg, fieldId);
			thresholds.add(t);
			fieldId = t.getNextFieldId();
		}
	}

	/**
    * Constructor for new data collection objects.
    *
    * @param owner The owner object
    * @param nodeId Owning node ID
    * @param id The table ID
    */
   public DataCollectionTable(DataCollectionConfiguration owner, long nodeId, long id)
	{
      super(owner, nodeId, id);
		instanceColumn = null;
		columns = new ArrayList<ColumnDefinition>(0);
		thresholds = new ArrayList<TableThreshold>(0);
	}

   /**
    * Constructor for new data collection objects.
    *
    * @param owner The owner object
    * @param id The table ID
    */
   public DataCollectionTable(DataCollectionConfiguration owner, long id)
   {
      this(owner, owner.getOwnerId(), id);
   }

   /**
    * Constructor for new data collection objects.
    *
    * @param nodeId Owning node ID
    * @param id The table ID
    */
   public DataCollectionTable(long nodeId, long id)
   {
      this(null, nodeId, id);
   }

   /**
    * Default constructor (intended for deserialization)
    */
   protected DataCollectionTable()
   {
      this(null, 0, 0);
   }

   /**
    * Object copy constructor
    *
    * @param owner object owner
    * @param src object to copy
    */
	protected DataCollectionTable(DataCollectionConfiguration owner, DataCollectionTable src)
   {
      super(owner, src);
      instanceColumn = src.instanceColumn;
      columns = new ArrayList<ColumnDefinition>(src.columns);
      thresholds = new ArrayList<TableThreshold>(src.thresholds);
   }

   /**
	 * Fill NXCP message with item's data.
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		super.fillMessage(msg);

		msg.setFieldInt16(NXCPCodes.VID_DCOBJECT_TYPE, DCO_TYPE_TABLE);
		msg.setField(NXCPCodes.VID_INSTANCE_COLUMN, instanceColumn);
		msg.setFieldInt32(NXCPCodes.VID_NUM_COLUMNS, columns.size());
		long varId = NXCPCodes.VID_DCI_COLUMN_BASE;
		for(int i = 0; i < columns.size(); i++)
		{
			columns.get(i).fillMessage(msg, varId);
			varId += 10;
		}
		msg.setFieldInt32(NXCPCodes.VID_NUM_THRESHOLDS, thresholds.size());
		varId = NXCPCodes.VID_DCI_THRESHOLD_BASE;
		for(int i = 0; i < thresholds.size(); i++)
		{			
			varId = thresholds.get(i).fillMessage(msg, varId);
		}
	}

	/**
	 * @return the instanceColumn
	 */
	public String getInstanceColumn()
	{
		return instanceColumn;
	}

	/**
	 * @param instanceColumn the instanceColumn to set
	 */
	public void setInstanceColumn(String instanceColumn)
	{
		this.instanceColumn = instanceColumn;
	}

	/**
	 * @return the columns
	 */
	public List<ColumnDefinition> getColumns()
	{
		return columns;
	}

	/**
	 * @param columns the columns to set
	 */
	public void setColumns(List<ColumnDefinition> columns)
	{
		this.columns = columns;
	}

	/**
	 * @return the thresholds
	 */
	public List<TableThreshold> getThresholds()
	{
		return thresholds;
	}

	/**
	 * @param thresholds the thresholds to set
	 */
	public void setThresholds(List<TableThreshold> thresholds)
	{
		this.thresholds = thresholds;
	}
}
