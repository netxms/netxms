/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

/**
 * Adapter class for interface <code>TextOutputListener</code> that implements all interface methods as no-op.
 */
public class TextOutputAdapter implements TextOutputListener
{
   /**
    * @see org.netxms.client.TextOutputListener#messageReceived(java.lang.String)
    */
   @Override
   public void messageReceived(String text)
   {
   }

   /**
    * @see org.netxms.client.TextOutputListener#setStreamId(long)
    */
   @Override
   public void setStreamId(long streamId)
   {
   }

   /**
    * @see org.netxms.client.TextOutputListener#onSuccess()
    */
   @Override
   public void onSuccess()
   {
   }

   /**
    * @see org.netxms.client.TextOutputListener#onFailure(java.lang.Exception)
    */
   @Override
   public void onFailure(Exception exception)
   {
   }
}
