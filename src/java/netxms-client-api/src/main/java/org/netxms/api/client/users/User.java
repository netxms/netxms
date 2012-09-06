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
package org.netxms.api.client.users;


import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * NetXMS user object
 * 
 */
public class User extends AbstractUserObject
{
	// Authentication methods
	public static final int AUTH_NETXMS_PASSWORD = 0;
	public static final int AUTH_RADIUS = 1;
	public static final int AUTH_CERTIFICATE = 2;
	public static final int AUTH_CERTIFICATE_OR_PASSWORD = 3;
	public static final int AUTH_CERTIFICATE_OR_RADIUS = 4;
	
	// Certificate mapping methods
	public static final int MAP_CERT_BY_SUBJECT		= 0;
	public static final int MAP_CERT_BY_PUBKEY		= 1;
	
	private int authMethod;
	private int certMappingMethod;
	private String certMappingData;
	private String fullName;
	private Date lastLogin = null;
	private Date lastPasswordChange = null;
	private int minPasswordLength;
	private Date disabledUntil = null;
	private int authFailures;

	/**
	 * Default constructor
	 */
	public User(final String name)
	{
		super(name);
		fullName = "";
	}
	
	/**
	 * Copy constructor
	 */
	public User(final User src)
	{
		super(src);
		
		this.authMethod = src.authMethod;
		this.certMappingMethod = src.certMappingMethod;
		this.certMappingData = src.certMappingData;
		this.fullName = src.fullName;
		this.lastLogin = src.lastLogin;
		this.lastPasswordChange = src.lastPasswordChange;
		this.minPasswordLength = src.minPasswordLength;
		this.disabledUntil = src.disabledUntil;
		this.authFailures = src.authFailures;
	}
	
	/**
	 * Create user object from NXCP message
	 */
	public User(final NXCPMessage msg)
	{
		super(msg);
		authMethod = msg.getVariableAsInteger(NXCPCodes.VID_AUTH_METHOD);
		fullName = msg.getVariableAsString(NXCPCodes.VID_USER_FULL_NAME);
		certMappingMethod = msg.getVariableAsInteger(NXCPCodes.VID_CERT_MAPPING_METHOD);
		certMappingData = msg.getVariableAsString(NXCPCodes.VID_CERT_MAPPING_DATA);
		lastLogin = msg.getVariableAsDate(NXCPCodes.VID_LAST_LOGIN);
		lastPasswordChange = msg.getVariableAsDate(NXCPCodes.VID_LAST_PASSWORD_CHANGE);
		minPasswordLength = msg.getVariableAsInteger(NXCPCodes.VID_MIN_PASSWORD_LENGTH);
		disabledUntil = msg.getVariableAsDate(NXCPCodes.VID_DISABLED_UNTIL);
		authFailures = msg.getVariableAsInteger(NXCPCodes.VID_AUTH_FAILURES);
	}
	
	/**
	 * Fill NXCP message with object data
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		super.fillMessage(msg);
		msg.setVariableInt16(NXCPCodes.VID_AUTH_METHOD, authMethod);
		msg.setVariable(NXCPCodes.VID_USER_FULL_NAME, fullName);
		msg.setVariableInt16(NXCPCodes.VID_CERT_MAPPING_METHOD, certMappingMethod);
		msg.setVariable(NXCPCodes.VID_CERT_MAPPING_DATA, certMappingData);
		msg.setVariableInt16(NXCPCodes.VID_MIN_PASSWORD_LENGTH, minPasswordLength);
		msg.setVariableInt32(NXCPCodes.VID_DISABLED_UNTIL, (disabledUntil != null) ? (int)(disabledUntil.getTime() / 1000) : 0);
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
	 * @return the certMappingMethod
	 */
	public int getCertMappingMethod()
	{
		return certMappingMethod;
	}

	/**
	 * @param certMappingMethod the certMappingMethod to set
	 */
	public void setCertMappingMethod(int certMappingMethod)
	{
		this.certMappingMethod = certMappingMethod;
	}

	/**
	 * @return the certMappingData
	 */
	public String getCertMappingData()
	{
		return certMappingData;
	}

	/**
	 * @param certMappingData the certMappingData to set
	 */
	public void setCertMappingData(String certMappingData)
	{
		this.certMappingData = certMappingData;
	}

	/**
	 * @return the fullName
	 */
	public String getFullName()
	{
		return fullName;
	}

	/**
	 * @param fullName the fullName to set
	 */
	public void setFullName(String fullName)
	{
		this.fullName = fullName;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#clone()
	 */
	@Override
	public Object clone() throws CloneNotSupportedException
	{
		return new User(this);
	}

	/**
	 * @return the minPasswordLength
	 */
	public int getMinPasswordLength()
	{
		return minPasswordLength;
	}

	/**
	 * @param minPasswordLength the minPasswordLength to set
	 */
	public void setMinPasswordLength(int minPasswordLength)
	{
		this.minPasswordLength = minPasswordLength;
	}

	/**
	 * @return the disabledUntil
	 */
	public Date getDisabledUntil()
	{
		return disabledUntil;
	}

	/**
	 * @param disabledUntil the disabledUntil to set
	 */
	public void setDisabledUntil(Date disabledUntil)
	{
		this.disabledUntil = disabledUntil;
	}

	/**
	 * @return the lastLogin
	 */
	public Date getLastLogin()
	{
		return lastLogin;
	}

	/**
	 * @return the lastPasswordChange
	 */
	public Date getLastPasswordChange()
	{
		return lastPasswordChange;
	}

	/**
	 * @return the authFailures
	 */
	public int getAuthFailures()
	{
		return authFailures;
	}
}
