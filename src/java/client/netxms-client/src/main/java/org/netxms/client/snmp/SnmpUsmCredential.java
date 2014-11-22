/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.client.snmp;

import org.netxms.base.NXCPMessage;

/**
 * @author Victor
 *
 */
public class SnmpUsmCredential
{
	private String name;
	private int authMethod;
	private int privMethod;
	private String authPassword;
	private String privPassword;
	
	/**
	 * Create credentials object from data in NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId Base variable ID
	 */
	public SnmpUsmCredential(NXCPMessage msg, long baseId)
	{
		name = msg.getFieldAsString(baseId);
		authMethod = msg.getFieldAsInt32(baseId + 1);
		privMethod = msg.getFieldAsInt32(baseId + 2);
		authPassword = msg.getFieldAsString(baseId + 3);
		privPassword = msg.getFieldAsString(baseId + 4);
	}
	
	/**
	 * Default constructor.
	 */
	public SnmpUsmCredential()
	{
		name = "";
		authMethod = 0;
		privMethod = 0;
		authPassword = "";
		privPassword = "";
	}
	
	/**
	 * Fill NXCP message with object's data
	 * 
	 * @param msg NXCP message
	 * @param baseId Base variable ID
	 */
	public void fillMessage(final NXCPMessage msg, long baseId)
	{
		msg.setField(baseId, name);
		msg.setFieldInt16(baseId + 1, authMethod);
		msg.setFieldInt16(baseId + 2, privMethod);
		msg.setField(baseId + 3, authPassword);
		msg.setField(baseId + 4, privPassword);
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the authMethod
	 */
	public int getAuthMethod()
	{
		return authMethod;
	}

	/**
	 * @param authMethod the authMethod to set
	 */
	public void setAuthMethod(int authMethod)
	{
		this.authMethod = authMethod;
	}

	/**
	 * @return the privMethod
	 */
	public int getPrivMethod()
	{
		return privMethod;
	}

	/**
	 * @param privMethod the privMethod to set
	 */
	public void setPrivMethod(int privMethod)
	{
		this.privMethod = privMethod;
	}

	/**
	 * @return the authPassword
	 */
	public String getAuthPassword()
	{
		return authPassword;
	}

	/**
	 * @param authPassword the authPassword to set
	 */
	public void setAuthPassword(String authPassword)
	{
		this.authPassword = authPassword;
	}

	/**
	 * @return the privPassword
	 */
	public String getPrivPassword()
	{
		return privPassword;
	}

	/**
	 * @param privPassword the privPassword to set
	 */
	public void setPrivPassword(String privPassword)
	{
		this.privPassword = privPassword;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode()
	{
		final int prime = 31;
		int result = 1;
		result = prime * result + authMethod;
		result = prime * result + ((authPassword == null) ? 0 : authPassword.hashCode());
		result = prime * result + ((name == null) ? 0 : name.hashCode());
		result = prime * result + privMethod;
		result = prime * result + ((privPassword == null) ? 0 : privPassword.hashCode());
		return result;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object obj)
	{
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		SnmpUsmCredential other = (SnmpUsmCredential)obj;
		if (authMethod != other.authMethod)
			return false;
		if (authPassword == null)
		{
			if (other.authPassword != null)
				return false;
		}
		else if (!authPassword.equals(other.authPassword))
			return false;
		if (name == null)
		{
			if (other.name != null)
				return false;
		}
		else if (!name.equals(other.name))
			return false;
		if (privMethod != other.privMethod)
			return false;
		if (privPassword == null)
		{
			if (other.privPassword != null)
				return false;
		}
		else if (!privPassword.equals(other.privPassword))
			return false;
		return true;
	}
}
