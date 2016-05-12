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

import java.io.InputStream;
import java.net.URL;
import java.util.HashSet;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.ui.IViewPart;
import org.json.JSONObject;
import org.json.JSONTokener;
import org.netxms.client.market.Repository;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.market.Activator;

/**
 * Repository runtime information
 */
public class RepositoryRuntimeInfo implements MarketObject
{
   private Repository repository;
   private Category events;
   private Category templates;
   private Category snmpTraps;
   private LoadingPlaceholder loadingPlaceholder;
   private boolean loaded;
   
   /**
    * Create repository info
    * 
    * @param repository
    */
   public RepositoryRuntimeInfo(Repository repository)
   {
      this.repository = repository;
      events = new Category("Events", this);
      templates = new Category("Templates", this);
      snmpTraps = new Category("SNMP Traps", this);
      loadingPlaceholder = new LoadingPlaceholder(this);
      loaded = false;
   }
   
   /**
    * Load repository objects
    * 
    * @param viewPart
    * @param completionHandler
    * @throws Exception
    */
   public void load(IViewPart viewPart, final Runnable completionHandler)
   {
      loaded = false;
      events.clear();
      templates.clear();
      snmpTraps.clear();
      
      ConsoleJob job = new ConsoleJob("Load repository objects", viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final Set<RepositoryElement> objects = loadRepositoryObjects();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  for(RepositoryElement e : objects)
                  {
                     if (e instanceof TemplateReference)
                        templates.add(e);
                     else if (e instanceof EventReference)
                        events.add(e);
                  }
                  
                  loaded = true;
                  completionHandler.run();
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot load repository objects";
         }
      };
      job.setUser(false);
      job.start();
   }
   
   /**
    * Load repository objects
    * 
    * @return
    * @throws Exception
    */
   private Set<RepositoryElement> loadRepositoryObjects() throws Exception
   {
      Set<RepositoryElement> objects = new HashSet<RepositoryElement>();
      URL url = new URL(repository.getUrl() + "/rest-api/get-available-items?accessToken=" + repository.getAuthToken());
      InputStream in = url.openStream();
      try
      {
         JSONTokener t = new JSONTokener(in);
         JSONObject root = new JSONObject(t);
         loadEvents(root, objects);
         loadTemplates(root, objects);
      }
      finally
      {
         in.close();
      }
      return objects;
   }
   
   /**
    * Load templates from JSON
    * 
    * @param root
    * @param objects
    * @throws Exception
    */
   private void loadTemplates(JSONObject root, Set<RepositoryElement> objects) throws Exception
   {
      if (root.isNull("templates"))
         return;
      JSONObject templates = root.getJSONObject("templates");
      for(String k : templates.keySet())
      {
         objects.add(new TemplateReference(UUID.fromString(k), templates.getJSONObject(k)));
      }
   }
   
   /**
    * Load events from JSON
    * 
    * @param root
    * @param objects
    * @throws Exception
    */
   private void loadEvents(JSONObject root, Set<RepositoryElement> objects) throws Exception
   {
      if (root.isNull("events"))
         return;
      JSONObject events = root.getJSONObject("events");
      for(String k : events.keySet())
      {
         objects.add(new EventReference(UUID.fromString(k), events.getJSONObject(k)));
      }
   }
   
   /**
    * @return
    */
   public boolean isLoaded()
   {
      return loaded;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#getName()
    */
   @Override
   public String getName()
   {
      return repository.getDescription();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#getGuid()
    */
   @Override
   public UUID getGuid()
   {
      return null;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#getParent()
    */
   @Override
   public MarketObject getParent()
   {
      return null;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#getChildren()
    */
   @Override
   public MarketObject[] getChildren()
   {
      return loaded ? new MarketObject[] { events, templates, snmpTraps } : new MarketObject[] { loadingPlaceholder };
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#hasChildren()
    */
   @Override
   public boolean hasChildren()
   {
      return true;
   }

   /**
    * Get repository ID
    * 
    * @return
    */
   public int getRepositoryId()
   {
      return repository.getId();
   }
}
