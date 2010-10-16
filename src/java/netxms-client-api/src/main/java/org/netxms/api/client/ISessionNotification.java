package org.netxms.api.client;

public interface ISessionNotification
{

	/**
	 * @return Notification's code
	 */
	public abstract int getCode();

	/**
	 * @return Notification's subcode
	 */
	public abstract long getSubCode();

	/**
	 * @return Object associated with notification
	 */
	public abstract Object getObject();

}