/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
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
   private String email;
   private String phoneNumber;
   private int[] groups;
   private Map<String, Map<String, String>> twoFactorAuthMethodBindings;

	/**
	 * Default constructor
	 *
	 * @param name user name
	 */
   public User(String name)
	{
		super(name, USERDB_TYPE_USER);
		fullName = "";
      email = "";
      phoneNumber = "";
      groups = new int[0];
      twoFactorAuthMethodBindings = new HashMap<String, Map<String, String>>(0);
	}

	/**
	 * Copy constructor
	 *
	 * @param src object for copy
	 */
   public User(User src)
	{
		super(src);
      authMethod = src.authMethod;
      certMappingMethod = src.certMappingMethod;
      certMappingData = src.certMappingData;
      fullName = src.fullName;
      lastLogin = src.lastLogin;
      lastPasswordChange = src.lastPasswordChange;
      minPasswordLength = src.minPasswordLength;
      disabledUntil = src.disabledUntil;
      authFailures = src.authFailures;
      email = src.email;
      phoneNumber = src.phoneNumber;
      groups = Arrays.copyOf(src.groups, src.groups.length);
      twoFactorAuthMethodBindings = new HashMap<String, Map<String, String>>(src.twoFactorAuthMethodBindings.size());
      for(Entry<String, Map<String, String>> e : src.twoFactorAuthMethodBindings.entrySet())
         twoFactorAuthMethodBindings.put(e.getKey(), new HashMap<String, String>(e.getValue()));
	}

	/**
	 * Create user object from NXCP message
	 *
	 * @param msg NXCP message
	 */
   public User(NXCPMessage msg)
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
      email = msg.getFieldAsString(NXCPCodes.VID_EMAIL);
      phoneNumber = msg.getFieldAsString(NXCPCodes.VID_PHONE_NUMBER);
      groups = msg.getFieldAsInt32Array(NXCPCodes.VID_GROUPS);
		if (groups == null)
         groups = new int[0];

      int count = msg.getFieldAsInt32(NXCPCodes.VID_2FA_METHOD_COUNT);
      twoFactorAuthMethodBindings = new HashMap<String, Map<String, String>>(count);
      long fieldId = NXCPCodes.VID_2FA_METHOD_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         String name = msg.getFieldAsString(fieldId);
         twoFactorAuthMethodBindings.put(name, msg.getStringMapFromFields(fieldId + 10, fieldId + 9));
         fieldId += 1000;
      }
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
      msg.setField(NXCPCodes.VID_EMAIL, email);
      msg.setField(NXCPCodes.VID_PHONE_NUMBER, phoneNumber);
		msg.setFieldInt32(NXCPCodes.VID_NUM_GROUPS, groups.length);
		if (groups.length > 0)
		   msg.setField(NXCPCodes.VID_GROUPS, groups);

      msg.setFieldInt32(NXCPCodes.VID_2FA_METHOD_COUNT, twoFactorAuthMethodBindings.size());
      long fieldId = NXCPCodes.VID_2FA_METHOD_LIST_BASE;
      for(Entry<String, Map<String, String>> e : twoFactorAuthMethodBindings.entrySet())
      {
         msg.setField(fieldId, e.getKey());
         msg.setFieldsFromStringMap(e.getValue(), fieldId + 10, fieldId + 9);
         fieldId += 1000;
      }
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
    * @see org.netxms.client.users.AbstractUserObject#getLabel()
    */
   @Override
   public String getLabel()
   {
      if (fullName.isEmpty())
         return super.getLabel();

      StringBuilder sb = new StringBuilder();
      sb.append(name);
      sb.append(" <");
      sb.append(fullName);
      if (!description.isEmpty())
      {
         sb.append("> (");
         sb.append(description);
         sb.append(')');
      }
      else
      {
         sb.append('>');
      }
      return sb.toString();
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
    * @return the email
    */
   public String getEmail()
   {
      return email;
   }

   /**
    * @param email the email to set
    */
   public void setEmail(String email)
   {
      this.email = email;
   }

   /**
    * @return the phoneNumber
    */
   public String getPhoneNumber()
   {
      return phoneNumber;
   }

   /**
    * @param phoneNumber the phoneNumber to set
    */
   public void setPhoneNumber(String phoneNumber)
   {
      this.phoneNumber = phoneNumber;
   }

   /**
    * @return the groups
    */
   public int[] getGroups()
   {
      return groups;
   }

   /**
    * @param groups the groups to set
    */
   public void setGroups(int[] groups)
   {
      this.groups = groups;
   }

   /**
    * @return the twoFactorAuthMethodBindings
    */
   public Map<String, Map<String, String>> getTwoFactorAuthMethodBindings()
   {
      return twoFactorAuthMethodBindings;
   }

   /**
    * @param twoFactorAuthMethodBindings the twoFactorAuthMethodBindings to set
    */
   public void setTwoFactorAuthMethodBindings(Map<String, Map<String, String>> twoFactorAuthMethodBindings)
   {
      this.twoFactorAuthMethodBindings = twoFactorAuthMethodBindings;
   }
}
