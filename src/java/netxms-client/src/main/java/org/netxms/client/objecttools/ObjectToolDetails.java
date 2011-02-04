/**
 * 
 */
package org.netxms.client.objecttools;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * @author victor
 *
 */
public class ObjectToolDetails extends ObjectTool
{
	private List<Long> accessList;
	private List<ObjectToolTableColumn> columns;
	
	/**
	 * Create object tool from NXCP message containing detailed tool information.
	 * Intended to be called only by NXCSession methods.
	 * 
	 * @param msg NXCP message
	 */
	public ObjectToolDetails(NXCPMessage msg)
	{
		id = msg.getVariableAsInt64(NXCPCodes.VID_TOOL_ID);
		name = msg.getVariableAsString(NXCPCodes.VID_NAME);
		type = msg.getVariableAsInteger(NXCPCodes.VID_TOOL_TYPE);
		data = msg.getVariableAsString(NXCPCodes.VID_TOOL_DATA);
		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		description = msg.getVariableAsString(NXCPCodes.VID_DESCRIPTION);
		snmpOid = msg.getVariableAsString(NXCPCodes.VID_TOOL_OID);
		confirmationText = msg.getVariableAsString(NXCPCodes.VID_CONFIRMATION_TEXT);
		
		accessList = new ArrayList<Long>(Arrays.asList(msg.getVariableAsUInt32ArrayEx(NXCPCodes.VID_ACL)));
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_COLUMNS);
		columns = new ArrayList<ObjectToolTableColumn>(count);
		long varId = NXCPCodes.VID_COLUMN_INFO_BASE;
		for(int i = 0; i < count; i++)
		{
			columns.add(new ObjectToolTableColumn(msg, varId));
			varId += 4;
		}

		createDisplayName();
	}
	
	/**
	 * Fill NXCP message with tool's data.
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(NXCPMessage msg)
	{
		msg.setVariableInt32(NXCPCodes.VID_TOOL_ID, (int)id);
		msg.setVariable(NXCPCodes.VID_NAME, name);
		msg.setVariable(NXCPCodes.VID_DESCRIPTION, description);
		msg.setVariable(NXCPCodes.VID_TOOL_OID, snmpOid);
		msg.setVariable(NXCPCodes.VID_CONFIRMATION_TEXT, confirmationText);
		msg.setVariable(NXCPCodes.VID_TOOL_DATA, data);
		msg.setVariableInt16(NXCPCodes.VID_TOOL_TYPE, type);

		msg.setVariableInt32(NXCPCodes.VID_ACL_SIZE, accessList.size());
		msg.setVariable(NXCPCodes.VID_ACL, accessList.toArray(new Long[accessList.size()]));
		
		msg.setVariableInt16(NXCPCodes.VID_NUM_COLUMNS, columns.size());
		long varId = NXCPCodes.VID_COLUMN_INFO_BASE;
		for(int i = 0; i < columns.size(); i++)
		{
			ObjectToolTableColumn c = columns.get(i);
			msg.setVariable(varId++, c.getName());
			msg.setVariable(varId++, c.getSnmpOid());
			msg.setVariableInt16(varId++, c.getFormat());
			msg.setVariableInt16(varId++, c.getSubstringIndex());
		}
	}

	/**
	 * @return the accessList
	 */
	public List<Long> getAccessList()
	{
		return accessList;
	}

	/**
	 * @return the columns
	 */
	public List<ObjectToolTableColumn> getColumns()
	{
		return columns;
	}

	/**
	 * @param id the id to set
	 */
	public void setId(long id)
	{
		this.id = id;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
		createDisplayName();
	}

	/**
	 * @param type the type to set
	 */
	public void setType(int type)
	{
		this.type = type;
	}

	/**
	 * @param flags the flags to set
	 */
	public void setFlags(int flags)
	{
		this.flags = flags;
	}

	/**
	 * @param description the description to set
	 */
	public void setDescription(String description)
	{
		this.description = description;
	}

	/**
	 * @param snmpOid the snmpOid to set
	 */
	public void setSnmpOid(String snmpOid)
	{
		this.snmpOid = snmpOid;
	}

	/**
	 * @param data the data to set
	 */
	public void setData(String data)
	{
		this.data = data;
	}

	/**
	 * @param confirmationText the confirmationText to set
	 */
	public void setConfirmationText(String confirmationText)
	{
		this.confirmationText = confirmationText;
	}
}
