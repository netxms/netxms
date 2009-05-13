/**
 * 
 */
package org.netxms.client;


import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * NetXMS user object
 * 
 * @author Victor
 *
 */
public class NXCUser extends NXCUserDBObject
{
	// Certificate mapping methods
	public static final int MAP_CERT_BY_SUBJECT		= 0;
	public static final int MAP_CERT_BY_PUBKEY		= 1;
	
	int authMethod;
	int certMappingMethod;
	String certMappingData;
	String fullName;

	/**
	 * Default constructor
	 */
	public NXCUser(final String name)
	{
		super(name);
		fullName = "";
	}
	
	/**
	 * Copy constructor
	 */
	public NXCUser(final NXCUser src)
	{
		super(src);
		
		this.authMethod = src.authMethod;
		this.certMappingMethod = src.certMappingMethod;
		this.certMappingData = new String(src.certMappingData);
		this.fullName = new String(src.fullName);
	}
	
	/**
	 * Create user object from NXCP message
	 */
	protected NXCUser(final NXCPMessage msg)
	{
		super(msg);
		authMethod = msg.getVariableAsInteger(NXCPCodes.VID_AUTH_METHOD);
		fullName = msg.getVariableAsString(NXCPCodes.VID_USER_FULL_NAME);
		certMappingMethod = msg.getVariableAsInteger(NXCPCodes.VID_CERT_MAPPING_METHOD);
		certMappingData = msg.getVariableAsString(NXCPCodes.VID_CERT_MAPPING_DATA);
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
}
