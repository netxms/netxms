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
package org.netxms.client.users;

import java.util.Arrays;
import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.CertificateMappingMethod;
import org.netxms.client.constants.UserAuthenticationMethod;

/**
 * NetXMS user object
 */
public class User extends AbstractUserObject
{
   private UserAuthenticationMethod authMethod;
   private CertificateMappingMethod certMappingMethod;
	private String certMappingData;
	private String fullName;
	private Date lastLogin = null;
	private Date lastPasswordChange = null;
	private int minPasswordLength;
	private Date disabledUntil = null;
	private int authFailures;
	private String xmppId;
   private long[] groups;

	/**
	 * Default constructor
	 *
	 * @param name user name
	 */
	public User(final String name)
	{
		super(name, USERDB_TYPE_USER);
		fullName = "";
		xmppId = "";
		groups = new long[0];
	}
	
	/**
	 * Copy constructor
	 *
	 * @param src object for copy
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
		this.xmppId = src.xmppId;
		this.groups = Arrays.copyOf(src.groups, src.groups.length);
	}
	
	/**
	 * Create user object from NXCP message
	 *
	 * @param msg NXCP message
	 */
	public User(final NXCPMessage msg)
	{
		super(msg, USERDB_TYPE_USER);
      authMethod = UserAuthenticationMethod.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_AUTH_METHOD));
		fullName = msg.getFieldAsString(NXCPCodes.VID_USER_FULL_NAME);
      certMappingMethod = CertificateMappingMethod.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_CERT_MAPPING_METHOD));
		certMappingData = msg.getFieldAsString(NXCPCodes.VID_CERT_MAPPING_DATA);
		lastLogin = msg.getFieldAsDate(NXCPCodes.VID_LAST_LOGIN);
		lastPasswordChange = msg.getFieldAsDate(NXCPCodes.VID_LAST_PASSWORD_CHANGE);
		minPasswordLength = msg.getFieldAsInt32(NXCPCodes.VID_MIN_PASSWORD_LENGTH);
		disabledUntil = msg.getFieldAsDate(NXCPCodes.VID_DISABLED_UNTIL);
		authFailures = msg.getFieldAsInt32(NXCPCodes.VID_AUTH_FAILURES);
		xmppId = msg.getFieldAsString(NXCPCodes.VID_XMPP_ID);
		groups = msg.getFieldAsUInt32Array(NXCPCodes.VID_GROUPS);
		if (groups == null)
		   groups = new long[0];
	}
	
	/**
	 * Fill NXCP message with object data
	 *
	 * @param msg NXCP message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		super.fillMessage(msg);
      msg.setFieldInt16(NXCPCodes.VID_AUTH_METHOD, authMethod.getValue());
		msg.setField(NXCPCodes.VID_USER_FULL_NAME, fullName);
      msg.setFieldInt16(NXCPCodes.VID_CERT_MAPPING_METHOD, certMappingMethod.getValue());
		msg.setField(NXCPCodes.VID_CERT_MAPPING_DATA, certMappingData);
		msg.setFieldInt16(NXCPCodes.VID_MIN_PASSWORD_LENGTH, minPasswordLength);
		msg.setFieldInt32(NXCPCodes.VID_DISABLED_UNTIL, (disabledUntil != null) ? (int)(disabledUntil.getTime() / 1000) : 0);
		msg.setField(NXCPCodes.VID_XMPP_ID, xmppId);
		msg.setFieldInt32(NXCPCodes.VID_NUM_GROUPS, groups.length);
		if (groups.length > 0)
		   msg.setField(NXCPCodes.VID_GROUPS, groups);
	}

	/**
    * Get authentication method for this user.
    *
    * @return authentication method
    */
   public UserAuthenticationMethod getAuthMethod()
	{
		return authMethod;
	}

	/**
    * Set authentication method for this user.
    *
    * @param authMethod new authentication method
    */
   public void setAuthMethod(UserAuthenticationMethod authMethod)
	{
		this.authMethod = authMethod;
	}

	/**
    * Get certificate mapping method for certificate based authentication.
    * 
    * @return certificate mapping method for certificate based authentication
    */
   public CertificateMappingMethod getCertMappingMethod()
	{
		return certMappingMethod;
	}

	/**
    * Set certificate mapping method for certificate based authentication.
    * 
    * @param certMappingMethod certificate mapping method for certificate based authentication
    */
   public void setCertMappingMethod(CertificateMappingMethod certMappingMethod)
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

   /**
    * @see org.netxms.client.users.AbstractUserObject#clone()
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

   /**
    * @return the xmppId
    */
   public String getXmppId()
   {
      return xmppId;
   }

   /**
    * @param xmppId the xmppId to set
    */
   public void setXmppId(String xmppId)
   {
      this.xmppId = xmppId;
   }

   /**
    * @return the groups
    */
   public long[] getGroups()
   {
      return groups;
   }

   /**
    * @param groups the groups to set
    */
   public void setGroups(long[] groups)
   {
      this.groups = groups;
   }
}
