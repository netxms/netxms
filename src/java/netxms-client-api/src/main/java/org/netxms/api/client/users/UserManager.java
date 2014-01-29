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

import java.io.IOException;
import org.netxms.api.client.NetXMSClientException;

/**
 * Interface for user management.
 *
 */
public interface UserManager
{
	// User object fields
	public static final int USER_MODIFY_LOGIN_NAME        = 0x00000001;
	public static final int USER_MODIFY_DESCRIPTION       = 0x00000002;
	public static final int USER_MODIFY_FULL_NAME         = 0x00000004;
	public static final int USER_MODIFY_FLAGS             = 0x00000008;
	public static final int USER_MODIFY_ACCESS_RIGHTS     = 0x00000010;
	public static final int USER_MODIFY_MEMBERS           = 0x00000020;
	public static final int USER_MODIFY_CERT_MAPPING      = 0x00000040;
	public static final int USER_MODIFY_AUTH_METHOD       = 0x00000080;
	public static final int USER_MODIFY_PASSWD_LENGTH     = 0x00000100;
	public static final int USER_MODIFY_TEMP_DISABLE      = 0x00000200;
	public static final int USER_MODIFY_CUSTOM_ATTRIBUTES = 0x00000400;
   public static final int USER_MODIFY_XMPP_ID           = 0x00000800;
   public static final int USER_MODIFY_GROUP_MEMBERSHIP  = 0x00001000;

	/**
	 * Synchronize user database
	 * 
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public abstract void syncUserDatabase() throws IOException, NetXMSClientException;

	/**
	 * Find user by ID
	 * 
	 * @return User object with given ID or null if such user does not exist
	 */
	public abstract AbstractUserObject findUserDBObjectById(final long id);

	/**
	 * Get list of all user database objects
	 * 
	 * @return List of all user database objects
	 */
	public abstract AbstractUserObject[] getUserDatabaseObjects();

	/**
	 * Create user on server
	 * 
	 * @param name
	 *           Login name for new user
	 * @return ID assigned to newly created user
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NetXMSClientException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public abstract long createUser(final String name) throws IOException, NetXMSClientException;

	/**
	 * Create user group on server
	 * 
	 * @param name
	 *           Name for new user group
	 * @return ID assigned to newly created user group
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NetXMSClientException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public abstract long createUserGroup(final String name) throws IOException, NetXMSClientException;

	/**
	 * Delete user or group on server
	 * 
	 * @param id
	 *           User or group ID
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NetXMSClientException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public abstract void deleteUserDBObject(final long id) throws IOException, NetXMSClientException;

	/**
	 * Set password for user
	 * 
	 * @param id User ID
	 * @param newPassword New password
	 * @param oldPassword Old password
	 * @throws IOException if socket I/O error occurs
	 * @throws NetXMSClientException if NetXMS server returns an error or operation was timed out
	 */
	public abstract void setUserPassword(final long id, final String newPassword, final String oldPassword) throws IOException,
			NetXMSClientException;

	/**
	 * Modify user database object
	 * 
	 * @param user
	 *           User data
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NetXMSClientException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public abstract void modifyUserDBObject(final AbstractUserObject object, final int fields) throws IOException, NetXMSClientException;

	/**
	 * Modify user database object
	 * 
	 * @param user
	 *           User data
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NetXMSClientException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public abstract void modifyUserDBObject(final AbstractUserObject object) throws IOException, NetXMSClientException;

	/**
	 * Lock user database
	 * 
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NetXMSClientException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public abstract void lockUserDatabase() throws IOException, NetXMSClientException;

	/**
	 * Unlock user database
	 * 
	 * @throws IOException if socket I/O error occurs
	 * @throws NetXMSClientException if NetXMS server returns an error or operation was timed out
	 */
	public abstract void unlockUserDatabase() throws IOException, NetXMSClientException;
}
