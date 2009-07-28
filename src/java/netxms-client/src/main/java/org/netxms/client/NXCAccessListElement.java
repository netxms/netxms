package org.netxms.client;

public final class NXCAccessListElement
{
	private long userId;
	private int accessRights;

	/**
	 * Create new ACL element with given user ID and rights
	 * 
	 * @param userId
	 * @param accessRights
	 */
	public NXCAccessListElement(long userId, int accessRights)
	{
		this.userId = userId;
		this.accessRights = accessRights;
	}
	
	/**
	 * Copy constructor
	 * 
	 * @param src Source ACL element
	 */
	public NXCAccessListElement(NXCAccessListElement src)
	{
		this.userId = src.userId;
		this.accessRights = src.accessRights;
	}

	/**
	 * @return the userId
	 */
	public long getUserId()
	{
		return userId;
	}

	/**
	 * @param accessRights the accessRights to set
	 */
	public void setAccessRights(int accessRights)
	{
		this.accessRights = accessRights;
	}

	/**
	 * @return the accessRights
	 */
	public int getAccessRights()
	{
		return accessRights;
	}
	
	/**
	 * @return true if READ access granted
	 */
	public boolean hasRead()
	{
		return (accessRights & NXCUserDBObject.OBJECT_ACCESS_READ) != 0;
	}
	
	/**
	 * @return true if MODIFY access granted
	 */
	public boolean hasModify()
	{
		return (accessRights & NXCUserDBObject.OBJECT_ACCESS_MODIFY) != 0;
	}
	
	/**
	 * @return true if DELETE access granted
	 */
	public boolean hasDelete()
	{
		return (accessRights & NXCUserDBObject.OBJECT_ACCESS_DELETE) != 0;
	}
	
	/**
	 * @return true if CREATE access granted
	 */
	public boolean hasCreate()
	{
		return (accessRights & NXCUserDBObject.OBJECT_ACCESS_CREATE) != 0;
	}
	
	/**
	 * @return true if READ ALARMS access granted
	 */
	public boolean hasReadAlarms()
	{
		return (accessRights & NXCUserDBObject.OBJECT_ACCESS_READ_ALARMS) != 0;
	}
	
	/**
	 * @return true if ACK ALARMS access granted
	 */
	public boolean hasAckAlarms()
	{
		return (accessRights & NXCUserDBObject.OBJECT_ACCESS_ACK_ALARMS) != 0;
	}
	
	/**
	 * @return true if TERMINATE ALARMS access granted
	 */
	public boolean hasTerminateAlarms()
	{
		return (accessRights & NXCUserDBObject.OBJECT_ACCESS_TERM_ALARMS) != 0;
	}
	
	/**
	 * @return true if CONTROL access granted
	 */
	public boolean hasControl()
	{
		return (accessRights & NXCUserDBObject.OBJECT_ACCESS_CONTROL) != 0;
	}
	
	/**
	 * @return true if SEND EVENTS access granted
	 */
	public boolean hasSendEvents()
	{
		return (accessRights & NXCUserDBObject.OBJECT_ACCESS_SEND_EVENTS) != 0;
	}
	
	/**
	 * @return true if ACCESS CONTROL access granted
	 */
	public boolean hasAccessControl()
	{
		return (accessRights & NXCUserDBObject.OBJECT_ACCESS_ACL) != 0;
	}
	
	/**
	 * @return true if PUSH DATA access granted
	 */
	public boolean hasPushData()
	{
		return (accessRights & NXCUserDBObject.OBJECT_ACCESS_PUSH_DATA) != 0;
	}
	
	@Override
	public boolean equals(Object aThat)
	{
		if (this == aThat)
			return true;
		
		if (!(aThat instanceof NXCAccessListElement))
			return false;
		
		return (this.userId == ((NXCAccessListElement)aThat).userId) &&
		       (this.accessRights == ((NXCAccessListElement)aThat).accessRights);
	}
	
	@Override
	public int hashCode()
	{
		return (int)((accessRights << 16) & userId);
	}
}
