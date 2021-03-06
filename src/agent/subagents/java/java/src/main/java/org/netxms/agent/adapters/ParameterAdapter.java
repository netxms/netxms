/**
 * Java-Bridge NetXMS subagent
 * Copyright (c) 2013 TEMPEST a.s.
 * Copyright (c) 2015 Raden Solutions SIA
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
package org.netxms.agent.adapters;

import org.netxms.agent.Parameter;
import org.netxms.agent.ParameterType;

/**
 * Adapter for Parameter interface
 */
public abstract class ParameterAdapter extends AbstractAdapter implements Parameter
{
   private String name;
   private String description;
   private ParameterType type;

   /**
    * @param name parameter name
    * @param description parameter description
    * @param type parameter type
    */
   public ParameterAdapter(String name, String description, ParameterType type)
   {
      this.name = name;
      this.description = description;
      this.type = type;
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.AgentContributionItem#getName()
    */
   @Override
   public String getName()
   {
      return name;
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.AgentContributionItem#getDescription()
    */
   @Override
   public String getDescription()
   {
      return description;
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Parameter#getType()
    */
   @Override
   public ParameterType getType()
   {
      return type;
   }
}
