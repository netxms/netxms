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
package org.netxms.client;

/**
 * Version of object access rights known to the session. Session replaces it with new instance when server reports newer
 * rights generation and on every login, so rights remembered together with any other instance are outdated.
 */
public final class ObjectRightsVersion
{
   private final long generation;

   /**
    * Create version for given server rights generation.
    *
    * @param generation server rights generation
    */
   ObjectRightsVersion(long generation)
   {
      this.generation = generation;
   }

   /**
    * Get server rights generation.
    *
    * @return server rights generation
    */
   public long getGeneration()
   {
      return generation;
   }
}
