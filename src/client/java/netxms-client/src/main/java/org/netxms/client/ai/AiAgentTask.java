/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2025 Raden Solutions
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
package org.netxms.client.ai;

import java.util.Date;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.AiTaskState;

/**
 * AI agent task
 */
public class AiAgentTask
{
   private long id;
   private int userId;
   private String description;
   private String prompt;
   private AiTaskState state;
   private Date lastExecutionTime;
   private Date nextExecutionTime;
   private int iteration;
   private String explanation;

   /**
    * Create task object from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base ID for fields
    */
   public AiAgentTask(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt64(baseId);
      userId = msg.getFieldAsInt32(baseId + 1);
      description = msg.getFieldAsString(baseId + 2);
      prompt = msg.getFieldAsString(baseId + 3);
      state = AiTaskState.getByValue(msg.getFieldAsInt32(baseId + 4));
      lastExecutionTime = msg.getFieldAsDate(baseId + 5);
      nextExecutionTime = msg.getFieldAsDate(baseId + 6);
      iteration = msg.getFieldAsInt32(baseId + 7);
      explanation = msg.getFieldAsString(baseId + 8);
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @return the userId
    */
   public int getUserId()
   {
      return userId;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @return the prompt
    */
   public String getPrompt()
   {
      return prompt;
   }

   /**
    * @return the state
    */
   public AiTaskState getState()
   {
      return state;
   }

   /**
    * @return the lastExecutionTime
    */
   public Date getLastExecutionTime()
   {
      return lastExecutionTime;
   }

   /**
    * @return the nextExecutionTime
    */
   public Date getNextExecutionTime()
   {
      return nextExecutionTime;
   }

   /**
    * @return the iteration
    */
   public int getIteration()
   {
      return iteration;
   }

   /**
    * @return the explanation
    */
   public String getExplanation()
   {
      return explanation;
   }
}
