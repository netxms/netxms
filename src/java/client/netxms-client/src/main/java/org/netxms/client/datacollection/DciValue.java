/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import java.text.NumberFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.IllegalFormatException;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.Severity;

/**
 * DCI value
 */
public abstract class DciValue
{
	private long id;					// DCI id
	private long nodeId;				// related node object id
	private long templateDciId;	// related template DCI ID
	private String name;				// name
	private String description;	// description
	private String value;			// value
	private int source;				// data source (agent, SNMP, etc.)
	private int dataType;
	private int status;				// status (active, disabled, etc.)
	private int errorCount;
	private int dcObjectType;		// Data collection object type (item, table, etc.)
	private Date timestamp;
	private Threshold activeThreshold;
	
	/**
	 * Factory method to create correct DciValue subclass from NXCP message.
	 * 
	 * @param nodeId owning node ID
	 * @param msg NXCP message
	 * @param base Base variable ID for value object
	 * @return DciValue object
	 */
	public static DciValue createFromMessage(long nodeId, NXCPMessage msg, long base)
	{
		int type = msg.getFieldAsInt32(base + 8);
		switch(type)
		{
			case DataCollectionObject.DCO_TYPE_ITEM:
				return new SimpleDciValue(nodeId, msg, base);
			case DataCollectionObject.DCO_TYPE_TABLE:
				return new TableDciValue(nodeId, msg, base);
			default:
				return null;
		}
	}
	
	/**
	 * Constructor for creating DciValue from NXCP message
	 * 
	 * @param nodeId owning node ID
	 * @param msg NXCP message
	 * @param base Base variable ID for value object
	 */
	protected DciValue(long nodeId, NXCPMessage msg, long base)
	{
		long fieldId = base;
	
		this.nodeId = nodeId;
		id = msg.getFieldAsInt64(fieldId++);
		name = msg.getFieldAsString(fieldId++);
		description = msg.getFieldAsString(fieldId++);
		source = msg.getFieldAsInt32(fieldId++);
		dataType = msg.getFieldAsInt32(fieldId++);
		value = msg.getFieldAsString(fieldId++);
		timestamp = msg.getFieldAsDate(fieldId++);
		status = msg.getFieldAsInt32(fieldId++);
		dcObjectType = msg.getFieldAsInt32(fieldId++);
		errorCount = msg.getFieldAsInt32(fieldId++);
		templateDciId = msg.getFieldAsInt64(fieldId++);
		if (msg.getFieldAsBoolean(fieldId++))
			activeThreshold = new Threshold(msg, fieldId);
		else
			activeThreshold = null;
	}
	
	/**
	 * Constructor for creating DCIValue from last values message
	 * 
	 * @param id
	 * @param value
	 * @param dataType
	 * @param status
	 */
	public DciValue(long id, String value, int dataType, int status)
	{
	   this.id = id;
	   this.value = value;
	   this.dataType = dataType;
	   this.status = status;
	}
	
	/**
	 * Class to hold value for formatter
	 */
	private class Value
	{
	   Object value;
	   String suffix;
	}
	
   /**
    * Get value ready for formatter
    * 
    * @param useMultipliers
    * @return
    */
   private Value getValueForFormat(boolean useMultipliers)
   {
      Value v = new Value();
      v.suffix = "";
      
      try
      {
         switch(dataType)
         {
            case DataCollectionObject.DT_INT:
            case DataCollectionObject.DT_UINT:
            case DataCollectionItem.DT_INT64:
            case DataCollectionItem.DT_UINT64:               
               if (useMultipliers)
               {
                  long i = Long.parseLong(value);
                  if ((i >= 10000000000000L) || (i <= -10000000000000L))
                  {
                     i = i / 1000000000000L;
                     v.suffix = "T";
                  }
                  if ((i >= 10000000000L) || (i <= -10000000000L))
                  {
                     i = i / 1000000000L;
                     v.suffix = "G";
                  }
                  if ((i >= 10000000) || (i <= -10000000))
                  {
                     i = i / 1000000;
                     v.suffix = "M";
                  }
                  if ((i >= 10000) || (i <= -10000))
                  {
                     i = i / 1000;
                     v.suffix = "K";
                  }
                  v.value = Long.valueOf(i);
               }
               else
               {
                  v.value = Long.parseLong(value);
               }
               break;
            case DataCollectionObject.DT_FLOAT:
               if (useMultipliers)
               {
                  double d = Double.parseDouble(value);
                  NumberFormat nf = NumberFormat.getNumberInstance();
                  nf.setMaximumFractionDigits(2);
                  if ((d >= 10000000000000.0) || (d <= -10000000000000.0))
                  {
                     d = d / 1000000000000.0;
                     v.suffix = "T";
                  }
                  if ((d >= 10000000000.0) || (d <= -10000000000.0))
                  {
                     d = d / 1000000000.0;
                     v.suffix = "G";
                  }
                  if ((d >= 10000000) || (d <= -10000000))
                  {
                     d = d / 1000000;
                     v.suffix = "M";
                  }
                  if ((d >= 10000) || (d <= -10000))
                  {
                     d = d / 1000;
                     v.suffix = "K";
                  }
                  v.value = Double.valueOf(d);
               }
               else
               {
                  v.value = Double.parseDouble(value);
               }
               break;
            default:
               v.value = value;
               break;
         }
      }     
      catch(NumberFormatException e)
      {
         v.value = value;
      }
      return v;
   }

   /**
	 * Returns formated DCI value or string with format error and correct type of DCI value;
	 * 
	 * @param formatString the string into which will be placed DCI value 
	 * @return
	 */
	public String format(String formatString)
	{
	   StringBuilder sb = new StringBuilder();
	   char[] format = formatString.toCharArray();
	   
	   for(int i = 0; i < format.length; i++)
	   {
	      if (format[i] == '%')
	      {
	         i++;
	         if (format[i] == '%')
	         {
	            sb.append('%');
	         }
	         else
	         {
	            int j;
	            for(j = i;(j < format.length) && !Character.isLetter(format[j]); j++);
	            
	            boolean useMultipliers = false;
	            if (format[i] == '*')
	            {
	               i++;
	               useMultipliers = true;
	            }
	            
	            final String f = "%" + new String(Arrays.copyOfRange(format, i, j + 1));
	            i = j;
	            
	            try
	            {
	               Value v = getValueForFormat(useMultipliers);
	               sb.append(String.format(f, v.value));
	               sb.append(v.suffix);
	            }
	            catch(IllegalFormatException e)
	            {
	               sb.append("<INVALID FORMAT> ("+f+")");
	            }
	         }
	      }
	      else
	      {
	         sb.append(format[i]);
	      }
	   }
	   return sb.toString();
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
		return name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the value
	 */
	public String getValue()
	{
		return value;
	}

	/**
	 * @return the source
	 */
	public int getSource()
	{
		return source;
	}

	/**
	 * @return the dataType
	 */
	public int getDataType()
	{
		return dataType;
	}

	/**
	 * @return the status
	 */
	public int getStatus()
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
}
