/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.ai;

import org.eclipse.jface.resource.ImageDescriptor;

/**
 * Context for AI assistant chat that is not a NetXMS object. Implementations are expected to build JSON representation on demand,
 * so that context content is always current at the moment of sending.
 */
public interface AiChatContext
{
   /**
    * Get context name to be displayed in chat control.
    *
    * @return context name
    */
   String getContextName();

   /**
    * Get image representing this context.
    *
    * @return image descriptor or null if context has no image
    */
   ImageDescriptor getContextImage();

   /**
    * Get JSON representation of this context to be sent to the server. Should be called at the moment of sending, because context
    * content may change over time.
    *
    * @return context as serialized JSON object
    */
   String getContextAsJson();
}
