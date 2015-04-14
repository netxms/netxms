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
import java.util.Comparator;
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
	private int numberOfColumns = 1;
	private List<ReportParameter> parameters = new ArrayList<ReportParameter>(0);
	
	/**
	 * Create report definition from NXCP message
	 * 
	 * @param msg
	 */
	public ReportDefinition(UUID id, NXCPMessage msg)
	{
      this.id = id;
      name = msg.getFieldAsString(NXCPCodes.VID_NAME);
      numberOfColumns = msg.getFieldAsInt32(NXCPCodes.VID_NUM_COLUMNS);
      int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      long varId = NXCPCodes.VID_ROW_DATA_BASE;
      for(int i = 0; i < count; i++, varId += 10)
      {
         parameters.add(new ReportParameter(msg, varId));
      }
      Collections.sort(parameters, new Comparator<ReportParameter>() {
         @Override
         public int compare(ReportParameter o1, ReportParameter o2)
         {
            return o1.getIndex() - o2.getIndex();
         }
      });
	}

	/**
	 * @return
	 */
	public UUID getId()
	{
		return id;
	}

	/**
	 * @return
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return
	 */
	public List<ReportParameter> getParameters()
	{
		return parameters;
	}

	/**
	 * @return
	 */
	public int getNumberOfColumns()
	{
		return numberOfColumns;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		return "ReportDefinition [id=" + id + ", name=" + name + ", numberOfColumns=" + numberOfColumns + ", parameters="
				+ parameters + "]";
	}
}
