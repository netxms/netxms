/**
 * NetXMS - open source network management system
 * Copyright (C) 2021-2023 Raden Solutions
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
package org.netxms.client;

import java.util.List;

/**
 * Login callback for implementing two factor authentication during login.
 */
public interface TwoFactorAuthenticationCallback
{
   /**
    * Select two-factor authentication method.
    *
    * @param methods list of available two-factor authentication methods
    * @return index of selected method or -1 to cancel login procedure
    */
   public int selectMethod(List<String> methods);

   /**
    * Get user response with selected authentication method. This method will receive non-null <code>qrLabel</code> if there is text
    * to be displayed to user as QR code. Usually this indicates that new secret was generated for selected authentication method
    * and should be presented to user.
    *
    * @param challenge optional challenge text (could be null)
    * @param qrLabel text to be displayed as QR code for scan (could be null)
    * @param trustedDevicesAllowed true if trusted devices are allowed by the server
    * @return user's response
    */
   public String getUserResponse(String challenge, String qrLabel, boolean trustedDevicesAllowed);

   /**
    * Save trusted device token.
    *
    * @param serverId server ID
    * @param username user name for which token should be provided
    * @param token trusted device token sent by server
    */
   public void saveTrustedDeviceToken(long serverId, String username, byte[] token);

   /**
    * Get trusted device token to be provided to the server.
    *
    * @param serverId server ID
    * @param username user name for which token should be provided
    * @return trusted device token to provide to the server or null
    */
   public byte[] getTrustedDeviceToken(long serverId, String username);
}
