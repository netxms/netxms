/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.mobile.agent;

/**
 * NetXMS mobile client exception. Used to report API call errors.
 */
public class MobileAgentException extends Exception
{
	private static final long serialVersionUID = -3247688667511123892L;
	
	private static final String[] errorMessages = {
      "Request completed successfully",
      "Component locked",
      "Access denied",
      "Invalid request",
      "Request timed out",
      "Request is out of state",
      "Database failure",
      "Invalid object ID",
      "Object already exist",
      "Communication failure",
      "System failure",
      "Invalid user ID",
      "Invalid argument",
      "Duplicate DCI",
      "Invalid DCI ID",
      "Out of memory",
      "Input/Output error",
      "Incompatible operation",
      "Object creation failed",
      "Loop in object relationship detected",
      "Invalid object name",
      "Invalid alarm ID",
      "Invalid action ID",
      "Operation in progress",
      "Copy operation failed for one or more DCI(s)",
      "Invalid or unknown event code",
      "No interfaces suitable for sending magic packet",
      "No MAC address on interface",
      "Command not implemented",
      "Invalid trap configuration record ID",
      "Requested data collection item is not supported by agent",
      "Client and server versions mismatch",
      "Error parsing package information file",
      "Package with specified properties already installed on server",
      "Package file already exist on server",
      "Server resource busy",
      "Invalid package ID",
      "Invalid IP address",
      "Action is used in event processing policy",
      "Variable not found",
      "Server uses incompatible version of communication protocol",
      "Address already in use",
      "Unable to select cipher",
      "Invalid public key",
      "Invalid session key",
      "Encryption is not supported by peer",
      "Server internal error",
      "Execution of external command failed",
      "Invalid object tool ID",
      "SNMP protocol error",
      "Incorrect regular expression",
      "Parameter is not supported by agent",
      "File I/O operation failed",
      "MIB file is corrupted",
      "File transfer operation already in progress",
      "Invalid job ID",
      "Invalid script ID",
      "Invalid script name",
      "Unknown map name",
      "Invalid map ID",
      "Account disabled",
      "No more grace logins",
      "Server connection broken",
      "Invalid agent configuration ID",
      "Server has lost connection with backend database",
      "Alarm is still open in helpdesk system",
      "Alarm is not in \"outstanding\" state",
      "DCI data source is not a push agent",
      "Error parsing configuration import file",
      "Configuration cannot be imported because of validation errors",
		"Invalid graph ID",
		"Local cryptographic provider failure",
		"Unsupported authentication type",
		"Bad certificate",
		"Invalid certificate ID",
		"SNMP failure",
		"Node has no support for layer 2 topology discovery",
		"Invalid persistent storage key",
		"Named instance not found",
		"Invalid event ID",
		"Operation cannot be completed due to agent error",
		"Unknown variable",
		"Requested resource not available",
		"Job cannot be cancelled",
		"Invalid policy ID",
		"Unknown log name",
		"Invalid log handle",
		"New password is too weak",
		"Password was used before",
		"Invalid session handle",
		"Node already is a member of a cluster",
		"Job cannot be put on hold",
		"Job on hold cannot be resumed",
		"Zone ID is already in use",
		"Invalid zone ID",
		"Cannot delete non-empty zone object",
		"No physical component data",
		"Invalid alarm note ID",
		"Encryption error"
	};
	
	private int errorCode;

	/**
	 * @param errorCode
	 */
	public MobileAgentException(int errorCode)
	{
		this.errorCode = errorCode;
	}

	/* (non-Javadoc)
	 * @see java.lang.Throwable#getMessage()
	 */
	@Override
	public String getMessage()
	{
		try
		{
			return errorMessages[errorCode];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return "Error " + Integer.toString(errorCode);
		}
	}

	/**
	 * @return the errorCode
	 */
	public final int getErrorCode()
	{
		return errorCode;
	}
}
