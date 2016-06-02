/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.market.objects;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.List;
import java.util.UUID;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Common interface for repository elements (templates, events, etc.)
 */
public abstract class RepositoryElement implements MarketObject
{
   private UUID guid;
   private String name;
   private MarketObject parent;
   private List<Instance> instances;
   private boolean marked;
   
   /**
    * Create element from JSON object
    * 
    * @param guid
    * @param json
    */
   public RepositoryElement(UUID guid, JSONObject json)
   {
      this.guid = guid;
      name = json.getString("name");
      parent = null;
      marked = false;
      
      JSONArray a = json.getJSONArray("instances");
      if (a != null)
      {
         instances = new ArrayList<Instance>(a.length());
         for(int i = 0; i < a.length(); i++)
         {
            instances.add(new Instance(a.getJSONObject(i)));
         }
         Collections.sort(instances, new Comparator<Instance>() {
            @Override
            public int compare(Instance o1, Instance o2)
            {
               return o2.getVersion() - o1.getVersion();
            }
         });
      }
      else
      {
         instances = new ArrayList<Instance>(0);
      }
   }   
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#getName()
    */
   @Override
   public String getName()
   {
      return marked ? name + " *" : name;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#getGuid()
    */
   @Override
   public UUID getGuid()
   {
      return guid;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#getParent()
    */
   @Override
   public MarketObject getParent()
   {
      return parent;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#getChildren()
    */
   @Override
   public MarketObject[] getChildren()
   {
      return new MarketObject[0];
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#hasChildren()
    */
   @Override
   public boolean hasChildren()
   {
      return false;
   }

   /**
    * Change parent for repository element
    * 
    * @param parent
    */
   public void setParent(Category parent)
   {
      this.parent = parent;
   }

   /**
    * @return the marked
    */
   public boolean isMarked()
   {
      return marked;
   }

   /**
    * @param marked the marked to set
    */
   public void setMarked(boolean marked)
   {
      this.marked = marked;
   }
   
   /**
    * Get all instances of this element
    * 
    * @return
    */
   public List<Instance> getInstances()
   {
      return instances;
   }
   
   /**
    * Get most actual instance of this element
    * 
    * @return
    */
   public Instance getActualInstance()
   {
      return instances.isEmpty() ? null : instances.get(0);
   }
   
   /**
    * Get version of most actual instance
    * 
    * @return
    */
   public int getActualVersion()
   {
      return instances.isEmpty() ? 0 : instances.get(0).getVersion();
   }
   
   /**
    * Repository element's instance
    */
   public class Instance
   {
      private Date timestamp;
      private int version;
      private String comments;
      
      /**
       * Create instance from JSON object
       * 
       * @param json
       */
      protected Instance(JSONObject json)
      {
         timestamp = new Date(json.getLong("timestamp") * 1000L);
         version = json.getInt("version");
         try
         {
            comments = json.getString("comment");
         }
         catch(JSONException e)
         {
            comments = "";
         }
      }

      /**
       * @return the timestamp
       */
      public Date getTimestamp()
      {
         return timestamp;
      }

      /**
       * @return the version
       */
      public int getVersion()
      {
         return version;
      }

      /**
       * @return the comments
       */
      public String getComments()
      {
         return comments;
      }
   }
}
