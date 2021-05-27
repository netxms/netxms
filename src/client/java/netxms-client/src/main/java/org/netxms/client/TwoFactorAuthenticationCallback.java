/**
 * NetXMS - open source network management system
 * Copyright (C) 2021 Raden Solutions
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
    * Get user response with selected authentication method.
    *
    * @param challenge optional challenge text (could be null)
    * @return user's response
    */
   public String getUserResponse(String challenge);
}
