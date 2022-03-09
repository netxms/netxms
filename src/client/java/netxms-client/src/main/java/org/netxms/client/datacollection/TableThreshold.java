/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.datacollection;

import java.util.ArrayList;
import java.util.List;

import org.netxms.base.NXCPMessage;

/**
 * Threshold definition for table DCI
 */
public class TableThreshold
{
   private static final String[] OPERATIONS = { "<", "<=", "==", ">=", ">", "!=", "LIKE", "NOT LIKE" };

   private long id;
   private int activationEvent;
   private int deactivationEvent;
   private int sampleCount;
   private List<List<TableCondition>> conditions;
   private long nextFieldId;

   /**
    * Create new empty threshold
    */
   public TableThreshold()
   {
      id = 0;
      activationEvent = 69;
      deactivationEvent = 70;
      sampleCount = 1;
      conditions = new ArrayList<List<TableCondition>>(0);
   }

   /**
    * Copy constructor
    *
    * @param src source object
    */
   public TableThreshold(TableThreshold src)
   {
      id = src.id;
      activationEvent = src.activationEvent;
      deactivationEvent = src.deactivationEvent;
      sampleCount = src.sampleCount;
      conditions = new ArrayList<List<TableCondition>>(src.conditions.size());
      for(List<TableCondition> sl : src.conditions)
      {
         List<TableCondition> dl = new ArrayList<TableCondition>(sl.size());
         for(TableCondition c : sl)
            dl.add(new TableCondition(c));
         conditions.add(dl);
      }
   }

   /**
    * Create from NXCP message
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   protected TableThreshold(NXCPMessage msg, long baseId)
   {
      long fieldId = baseId;
      id = msg.getFieldAsInt64(fieldId++);
      activationEvent = msg.getFieldAsInt32(fieldId++);
      deactivationEvent = msg.getFieldAsInt32(fieldId++);
      sampleCount = msg.getFieldAsInt32(fieldId++);

      int groupCount = msg.getFieldAsInt32(fieldId++);
      conditions = new ArrayList<List<TableCondition>>(groupCount);
      for(int i = 0; i < groupCount; i++)
      {
         int condCount = msg.getFieldAsInt32(fieldId++);
         List<TableCondition> list = new ArrayList<TableCondition>(condCount);
         for(int j = 0; j < condCount; j++)
         {
            list.add(new TableCondition(msg, fieldId));
            fieldId += 3;
         }
         conditions.add(list);
      }
      nextFieldId = fieldId;
   }

   /**
    * Fill NXCP message with threshold data
    *
    * @param msg NXCP message
    * @param baseId base field ID
    * @return next free field ID
    */
   protected long fillMessage(NXCPMessage msg, long baseId)
   {
      long fieldId = baseId;

      msg.setFieldInt32(fieldId++, (int)id);
      msg.setFieldInt32(fieldId++, activationEvent);
      msg.setFieldInt32(fieldId++, deactivationEvent);
      msg.setFieldInt32(fieldId++, sampleCount);
      msg.setFieldInt32(fieldId++, conditions.size());
      for(List<TableCondition> l : conditions)
      {
         msg.setFieldInt32(fieldId++, l.size());
         for(TableCondition c : l)
         {
            msg.setField(fieldId++, c.getColumn());
            msg.setFieldInt16(fieldId++, c.getOperation());
            msg.setField(fieldId++, c.getValue());
         }
      }

      return fieldId;
   }

   /**
    * Get threshold condition as text
    *
    * @return textual representation of threshold condition
    */
   public String getConditionAsText()
   {
      StringBuilder sb = new StringBuilder();
      for(List<TableCondition> group : conditions)
      {
         if (sb.length() > 0)
            sb.append(" OR ");
         if (conditions.size() > 1)
            sb.append('(');
         for(TableCondition cond : group)
         {
            if (cond != group.get(0))
               sb.append(" AND ");
            sb.append(cond.getColumn());
            sb.append(' ');
            try
            {
               sb.append(OPERATIONS[cond.getOperation()]);
            }
            catch(ArrayIndexOutOfBoundsException e)
            {
               sb.append('?');
            }
            sb.append(' ');
            sb.append(cond.getValue());
         }
         if (conditions.size() > 1)
            sb.append(')');
      }
      return sb.toString();
   }

   /**
    * Get next available field ID
    *
    * @return next available field ID
    */
   public long getNextFieldId()
   {
      return nextFieldId;
   }

   /**
    * Get activation event code
    *
    * @return activation event code
    */
   public int getActivationEvent()
   {
      return activationEvent;
   }

   /**
    * Set activation event code
    *
    * @param activationEvent new activation event code
    */
   public void setActivationEvent(int activationEvent)
   {
      this.activationEvent = activationEvent;
   }

   /**
    * Get deactivation event code
    *
    * @return deactivation event code
    */
   public int getDeactivationEvent()
   {
      return deactivationEvent;
   }

   /**
    * Set deactivation event code
    * @param deactivationEvent new deactivation event code
    */
   public void setDeactivationEvent(int deactivationEvent)
   {
      this.deactivationEvent = deactivationEvent;
   }

   /**
    * Get sample count
    *
    * @return sample count
    */
   public int getSampleCount()
   {
      return sampleCount;
   }

   /**
    * Set sample count
    *
    * @param sampleCount new sample count
    */
   public void setSampleCount(int sampleCount)
   {
      this.sampleCount = sampleCount;
   }

   /**
    * @return the conditions
    */
   public List<List<TableCondition>> getConditions()
   {
      return conditions;
   }

   /**
    * @param conditions the conditions to set
    */
   public void setConditions(List<List<TableCondition>> conditions)
   {
      this.conditions = conditions;
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * Duplicate threshold for later use 
    * (do not copy ID for duplicated threshold)
    * 
    * @return threshold copy
    */
   public TableThreshold duplicate()
   {
      TableThreshold tr = new TableThreshold(this);
      tr.id = 0;
      return tr;
   }
}
