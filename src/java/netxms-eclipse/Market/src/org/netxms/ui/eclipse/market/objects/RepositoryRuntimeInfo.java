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

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.ui.IViewPart;
import org.json.JSONObject;
import org.json.JSONTokener;
import org.netxms.base.NXCommon;
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
   private Category rules;
   private Category scripts;
   private Category snmpTraps;
   private Category summaryTables;
   private Category templates;
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
      rules = new Category("Rules", this);
      scripts = new Category("Scripts", this);
      snmpTraps = new Category("SNMP Traps", this);
      summaryTables = new Category("Summary Tables", this);
      templates = new Category("Templates", this);
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
      rules.clear();
      scripts.clear();
      snmpTraps.clear();
      summaryTables.clear();
      templates.clear();
      
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
         Activator.logInfo("JSON received for repository " + repository.getDescription() + " (" + repository.getUrl() + "): " + root.toString());
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
      return loaded ? new MarketObject[] { events, rules, scripts, snmpTraps, summaryTables, templates } : new MarketObject[] { loadingPlaceholder };
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
   
   /**
    * Set/remove mark on all repository elements
    * 
    * @param marked
    */
   public void setAllMarked(boolean marked)
   {
      if (!loaded)
         return;
      
      for(MarketObject o : getChildren())
      {
         for(MarketObject e : o.getChildren())
         {
            if (e instanceof RepositoryElement)
               ((RepositoryElement)e).setMarked(marked);
         }
      }
   }
   
   /**
    * Get all marked elements within repository
    * 
    * @return
    */
   public List<RepositoryElement> getMarkedElements()
   {
      List<RepositoryElement> list = new ArrayList<RepositoryElement>();
      if (!loaded)
         return list;

      for(MarketObject o : getChildren())
      {
         for(MarketObject e : o.getChildren())
         {
            if ((e instanceof RepositoryElement) && ((RepositoryElement)e).isMarked())
               list.add((RepositoryElement)e);
         }
      }
      return list;
   }
   
   /**
    * Load import file from repository
    * 
    * @param request
    * @return
    * @throws Exception
    */
   public String loadImportFile(String request) throws Exception
   {
      StringBuilder content = new StringBuilder();
      URL url = new URL(repository.getUrl() + "/rest-api/get-items?accessToken=" + repository.getAuthToken());
      
      OutputStream out = null;
      BufferedReader in = null;
      
      URLConnection conn = url.openConnection();
      if (!(conn instanceof HttpURLConnection))
      {
         throw new Exception("Unsupported URL type");
      }
      ((HttpURLConnection)conn).setRequestMethod("POST");
      ((HttpURLConnection)conn).setRequestProperty("User-Agent", "NetXMS Console/" + NXCommon.VERSION);
      ((HttpURLConnection)conn).setRequestProperty("Content-Type", "application/json; charset=utf-8");
      ((HttpURLConnection)conn).setDoOutput(true);
      ((HttpURLConnection)conn).setAllowUserInteraction(false);
      ((HttpURLConnection)conn).setUseCaches(false);
      
      try
      {
         out = conn.getOutputStream();
         out.write(request.getBytes("utf-8"));
         
         in = new BufferedReader(new InputStreamReader(conn.getInputStream()));
         String line;
         while((line = in.readLine()) != null)
         {
            content.append(line);
         }
      }
      finally
      {
         if (out != null)
            out.close();
         if (in != null)
            in.close();
      }
      return content.toString();
   }
}
