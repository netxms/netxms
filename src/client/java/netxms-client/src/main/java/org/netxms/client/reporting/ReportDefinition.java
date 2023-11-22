/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
package org.netxms.client.reporting;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Report definition
 */
public class ReportDefinition
{
	private UUID id;
	private String name;
   private int numberOfColumns;
   private List<ReportParameter> parameters;
   private boolean valid;

	/**
	 * Create report definition from NXCP message
	 * 
	 * @param id report definition ID
	 * @param msg NXCP message
	 */
	public ReportDefinition(UUID id, NXCPMessage msg)
	{
      this.id = id;
      name = msg.getFieldAsString(NXCPCodes.VID_NAME);
      numberOfColumns = msg.getFieldAsInt32(NXCPCodes.VID_NUM_COLUMNS);
      int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      parameters = new ArrayList<ReportParameter>(count);
      long fieldId = NXCPCodes.VID_ROW_DATA_BASE;
      for(int i = 0; i < count; i++, fieldId += 10)
         parameters.add(new ReportParameter(msg, fieldId));
      Collections.sort(parameters, (p1, p2) -> p1.getIndex() - p2.getIndex());
      valid = true;
	}

   /**
    * Create report definition for invalid report.
    *
    * @param id report definition ID
    */
   public ReportDefinition(UUID id)
   {
      this.id = id;
      name = id.toString();
      numberOfColumns = 1;
      parameters = new ArrayList<ReportParameter>(0);
      valid = false;
   }

	/**
	 * Get report definition ID
	 * 
	 * @return report definition ID
	 */
	public UUID getId()
	{
		return id;
	}

	/**
	 * Get report definition name
	 * 
	 * @return report definition name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * Get report parameters
	 * 
	 * @return list of report parameters
	 */
	public List<ReportParameter> getParameters()
	{
		return parameters;
	}

	/**
	 * Get number of columns in input form
	 * 
	 * @return number of columns in input form
	 */
	public int getNumberOfColumns()
	{
		return numberOfColumns;
	}

   /**
    * @return the valid
    */
   public boolean isValid()
   {
      return valid;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "ReportDefinition [id=" + id + ", name=" + name + ", numberOfColumns=" + numberOfColumns + ", parameters=" + parameters + ", valid=" + valid + "]";
   }
}
