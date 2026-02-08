/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import java.util.Date;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.DataCollectionObjectStatus;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.Severity;

/**
 * DCI value
 */
public abstract class DciValue
{
   final public static int MULTIPLIERS_DEFAULT = 0;
   final public static int MULTIPLIERS_YES = 1;
   final public static int MULTIPLIERS_NO = 2;

	protected long id;					// DCI id
	protected long nodeId;				// related node object id
   protected long templateDciId; // related template DCI ID
	protected String name;				// name
	protected String description;	// description
	protected String value;			// value
   protected String userTag;
   protected String comments;
   protected DataOrigin source;  // data source (agent, SNMP, etc.)
	protected DataType dataType;
   protected DataCollectionObjectStatus status;
   protected int errorCount;
   protected int dcObjectType; // Data collection object type (item, table, etc.)
   protected Date timestamp;
   protected Threshold activeThreshold;
   protected int flags;
   protected MeasurementUnit measurementUnit;
   protected int multiplier;
   protected boolean noValueObject;
   protected boolean anomalyDetected;
   protected long thresholdDisableEndTime;

   /**
    * Factory method to create correct DciValue subclass from NXCP message.
    * 
    * @param msg NXCP message
    * @param base Base variable ID for value object
    * @return DciValue object
    */
	public static DciValue createFromMessage(NXCPMessage msg, long base)
	{
		int type = msg.getFieldAsInt32(base + 10);
		switch(type)
		{
			case DataCollectionObject.DCO_TYPE_ITEM:
				return new SimpleDciValue(msg, base);
			case DataCollectionObject.DCO_TYPE_TABLE:
				return new TableDciValue(msg, base);
			default:
				return null;
		}
	}

	/**
	 * Simple constructor for DciValue
	 */
	protected DciValue()
   {
   }

	/**
	 * Constructor for creating DciValue from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param base Base field ID for value object
	 */
	protected DciValue(NXCPMessage msg, long base)
	{
		long fieldId = base;

      nodeId = msg.getFieldAsInt64(fieldId++);
		id = msg.getFieldAsInt64(fieldId++);
		name = msg.getFieldAsString(fieldId++);
      flags = msg.getFieldAsInt32(fieldId++);
		description = msg.getFieldAsString(fieldId++);
      source = DataOrigin.getByValue(msg.getFieldAsInt32(fieldId++));
		dataType = DataType.getByValue(msg.getFieldAsInt32(fieldId++));
		value = msg.getFieldAsString(fieldId++);
      timestamp = msg.getFieldAsTimestamp(fieldId++);
      status = DataCollectionObjectStatus.getByValue(msg.getFieldAsInt32(fieldId++));
		dcObjectType = msg.getFieldAsInt32(fieldId++);
		errorCount = msg.getFieldAsInt32(fieldId++);
		templateDciId = msg.getFieldAsInt64(fieldId++);
      measurementUnit = new MeasurementUnit(msg, fieldId++);
      multiplier = msg.getFieldAsInt32(fieldId++);
      noValueObject = msg.getFieldAsBoolean(fieldId++);
      comments = msg.getFieldAsString(fieldId++);
      anomalyDetected = msg.getFieldAsBoolean(fieldId++);
      userTag = msg.getFieldAsString(fieldId++);
      thresholdDisableEndTime = msg.getFieldAsInt64(fieldId++);
		if (msg.getFieldAsBoolean(fieldId++))
			activeThreshold = new Threshold(msg, fieldId);
		else
			activeThreshold = null;
	}
   
   /**
    * Get data formatter for this DCI value.
    * 
    * @return data formatter
    */
   public DataFormatter createDataFormatter()
   {
      return new DataFormatter(this);
   }

   /**
    * @param timeFormatter
    * @param useMultipliers
    * @return
    */
   public String getFormattedValue(boolean useMultipliers, TimeFormatter timeFormatter)
   {
      return createDataFormatter().setDefaultForMultipliers(useMultipliers).format(value, timeFormatter);
   }

   /**
    * @param formatString
    * @param timeFormatter
    * @return
    */
   public Object getFormattedValue(String formatString, TimeFormatter timeFormatter)
   {
      return createDataFormatter().setFormatString(formatString).format(value, timeFormatter);
   }

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name.isEmpty() ? "[" + getId() + "]" : name;
	}

	/**
    * Get DCi description.
    *
    * @return DCI description
    */
	public String getDescription()
	{
		return description;
	}

	/**
    * Get current (last collected) DCI value.
    *
    * @return current DCI value
    */
	public String getValue()
	{
		return value;
	}

	/**
    * @return the userTag
    */
   public String getUserTag()
   {
      return userTag;
   }

   /**
    * Get DCI comments.
    *
    * @return DCI comments
    */
   public String getComments()
   {
      return comments;
   }

   /**
    * @return the source
    */
   public DataOrigin getSource()
	{
		return source;
	}

	/**
	 * @return the dataType
	 */
	public DataType getDataType()
	{
		return dataType;
	}

	/**
	 * @return the status
	 */
   public DataCollectionObjectStatus getStatus()
	{
		return status;
	}

	/**
	 * @return the timestamp
	 */
	public Date getTimestamp()
	{
		return timestamp;
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the activeThreshold
	 */
	public Threshold getActiveThreshold()
	{
		return activeThreshold;
	}

	/**
	 * @return the dcObjectType
	 */
	public int getDcObjectType()
	{
		return dcObjectType;
	}

	/**
	 * @return the errorCount
	 */
	public int getErrorCount()
	{
		return errorCount;
	}

	/**
	 * @return the templateDciId
	 */
	public final long getTemplateDciId()
	{
		return templateDciId;
	}

   /**
    * Get severity of active threshold
    * 
    * @return severity of active threshold or NORMAL if there are no active thresholds
    */
   public Severity getThresholdSeverity()
   {
      return (activeThreshold != null) ? activeThreshold.getCurrentSeverity() : Severity.NORMAL;
   }

   /**
    * @return the flags
    */
   public int getFlags()
   {
      return flags;
   }
   
   /**
    * Get multipliers selection.
    * 
    * @return multiplier usage mode (DEFAULT, YES, or NO)
    */
   public int getMultipliersSelection()
   {
      return (int)((flags & DataCollectionItem.DCF_MULTIPLIERS_MASK) >> 16);
   }
   
   /**
    * @return the mostCriticalSeverity
    */
   public Severity getMostCriticalSeverity()
   {
      return getThresholdSeverity();
   }
   
   /**
    * @return the unitName
    */
   public MeasurementUnit getMeasurementUnit()
   {
      return measurementUnit;
   }

   /**
    * @return the multiplier
    */
   public int getMultiplier()
   {
      return multiplier;
   }

   /**
    * @return the noValueObject
    */
   public boolean isNoValueObject()
   {
      return noValueObject;
   }

   /**
    * @return the anomalyDetected
    */
   public boolean isAnomalyDetected()
   {
      return anomalyDetected;
   }

   /**
    * Get time until which threshold processing is disabled for this DCI. Value of 0 means that threshold processing is enabled.
    * Value of -1 means that threshold processing is disabled permanently.
    *
    * @return the thresholdDisableEndTime time in seconds since epoch until which threshold processing is disabled
    */
   public long getThresholdDisableEndTime()
   {
      return thresholdDisableEndTime;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "DciValue [id=" + id + ", nodeId=" + nodeId + ", templateDciId=" + templateDciId + ", name=" + name + ", description=" + description + ", value=" + value + ", comments=" + comments +
            ", source=" + source + ", dataType=" + dataType + ", status=" + status + ", errorCount=" + errorCount + ", dcObjectType=" + dcObjectType + ", timestamp=" + timestamp +
            ", activeThreshold=" + activeThreshold + ", flags=" + flags + ", measurementUnit=" + measurementUnit + ", multiplier=" + multiplier + ", noValueObject=" + noValueObject +
            ", anomalyDetected=" + anomalyDetected + "]";
   }
}
