/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.client.objects;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.reports.ReportParameter;

/**
 * Report
 */
public class Report extends GenericObject
{
	private List<ReportParameter> parameters;
	
	/**
	 * @param msg
	 * @param session
	 */
	public Report(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_PARAMETERS);
		parameters = new ArrayList<ReportParameter>(count);
		long varId = NXCPCodes.VID_PARAM_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			parameters.add(new ReportParameter(msg, varId));
			varId += 10;
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Report";
	}

	/**
	 * Get report's parameters. This method returns reference to internal
	 * parameters list, so it should not be modified by caller.
	 * 
	 * @return the parameters
	 */
	public List<ReportParameter> getParameters()
	{
		return parameters;
	}
}
