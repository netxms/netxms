/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.client.datacollection;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Result of an ad-hoc web service request (returned by {@link org.netxms.client.NXCSession#queryWebService}).
 */
public class WebServiceCallResult
{
   private final int httpResponseCode;
   private final String document;

   /**
    * Create result from NXCP message.
    *
    * @param msg NXCP message
    */
   public WebServiceCallResult(NXCPMessage msg)
   {
      httpResponseCode = msg.getFieldAsInt32(NXCPCodes.VID_WEBSVC_RESPONSE_CODE);
      document = msg.getFieldAsString(NXCPCodes.VID_WEBSVC_RESPONSE);
   }

   /**
    * Get HTTP response code returned by the web service.
    *
    * @return HTTP response code
    */
   public int getHttpResponseCode()
   {
      return httpResponseCode;
   }

   /**
    * Get returned document (response body).
    *
    * @return returned document
    */
   public String getDocument()
   {
      return document;
   }
}
