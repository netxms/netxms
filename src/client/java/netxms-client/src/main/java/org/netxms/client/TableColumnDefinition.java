/**
 * 
 */
package org.netxms.client;

import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.MeasurementUnit;

/**
 * Table column definition
 */
public class TableColumnDefinition
{
	private String name;
	private String displayName;
	private DataType dataType;
	private boolean instanceColumn;
   private MeasurementUnit measurementUnit;
   private int useMultiplier = 0;  // DciValue.MULTIPLIERS_DEFAULT, DciValue.MULTIPLIERS_ON, DciValue.MULTIPLIERS_OFF
   private int multiplierPower = 0;
	
	/**
	 * @param name The name to set
	 * @param displayName The display name to set
	 * @param dataType The data type to set
	 * @param instanceColumn Set true if instance column
	 */
	public TableColumnDefinition(String name, String displayName, DataType dataType, boolean instanceColumn)
	{
		this.name = name;
		this.displayName = (displayName != null) ? displayName : name;
		this.dataType = dataType;
		this.instanceColumn = instanceColumn;
      this.measurementUnit = null;
	}

	/**
	 * @param msg The NXCPMessage
	 * @param baseId The base ID
	 */
	protected TableColumnDefinition(NXCPMessage msg, long baseId)
	{
		name = msg.getFieldAsString(baseId);
		dataType = DataType.getByValue(msg.getFieldAsInt32(baseId + 1));
		displayName = msg.getFieldAsString(baseId + 2);
		if (displayName == null)
			displayName = name;
		instanceColumn = msg.getFieldAsBoolean(baseId + 3);
      measurementUnit = new MeasurementUnit(msg, baseId + 4);
      multiplierPower = msg.getFieldAsInt32(baseId + 5);
      useMultiplier = msg.getFieldAsInt32(baseId + 6);
	}

	/**
	 * @param msg The NXCPMessage
	 * @param baseId The base ID
	 */
	protected void fillMessage(NXCPMessage msg, long baseId)
	{
		msg.setField(baseId, name);
		msg.setFieldInt32(baseId + 1, dataType.getValue());
		msg.setField(baseId + 2, displayName);
		msg.setFieldInt16(baseId + 3, instanceColumn ? 1 : 0);
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the displayName
	 */
	public String getDisplayName()
	{
		return displayName;
	}

	/**
	 * @return the dataType
	 */
	public DataType getDataType()
	{
		return dataType;
	}

	/**
	 * @return the instanceColumn
	 */
	public boolean isInstanceColumn()
	{
		return instanceColumn;
	}

   /**
    * @return the unitName
    */
   public MeasurementUnit getMeasurementUnit()
   {
      return measurementUnit;
   }

   /**
    * Get multiplier usage setting
    * 
    * @return multiplier usage setting
    */
   public int getUseMultiplier()
   {
      return useMultiplier;
   }

   /**
    * Get multiplier power
    * 
    * @return multiplier power
    */
   public int getMultiplierPower()
   {
      return multiplierPower;
   }
}
