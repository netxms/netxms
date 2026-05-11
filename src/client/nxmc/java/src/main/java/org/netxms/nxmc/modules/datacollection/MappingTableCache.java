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
package org.netxms.nxmc.modules.datacollection;

import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.CopyOnWriteArrayList;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.mt.MappingTable;
import org.netxms.nxmc.DisposableSingleton;
import org.netxms.nxmc.Registry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Per-session cache of {@link MappingTable} objects for DCI value rendering. Tables are fetched lazily on first use and
 * invalidated automatically on server-side change/delete notifications. Views that render mapped values can register a
 * {@link Runnable} via {@link #addLoadListener} to be re-rendered after a table loads.
 *
 * <p>This cache is per-session by design: in the web client multiple users share one JVM, so a JVM-wide cache would leak
 * mapped values between users connected to different servers (or with different access rights). The instance is stored
 * via {@link Registry#setSingleton}, which is per-RAP-UI-session in the web client and per-JVM in the desktop client.
 */
public class MappingTableCache implements DisposableSingleton
{
   private static final Logger logger = LoggerFactory.getLogger(MappingTableCache.class);

   private final NXCSession session;
   private final ConcurrentMap<Integer, MappingTable> cache = new ConcurrentHashMap<>();
   private final Set<Integer> pending = ConcurrentHashMap.newKeySet();
   private final List<Runnable> loadListeners = new CopyOnWriteArrayList<>();
   private final SessionListener sessionListener;

   /**
    * Attach cache to session. Should be called once per UI session right after login, alongside the other per-session
    * caches in {@code Startup}.
    *
    * @param display display owning the UI session
    * @param session current client session
    */
   public static void attachSession(Display display, NXCSession session)
   {
      Registry.setSingleton(display, MappingTableCache.class, new MappingTableCache(session));
   }

   /**
    * Get the cache instance for the current UI session. Must be called from the UI thread.
    *
    * @return cache instance, or {@code null} if not yet attached (e.g. before login completes)
    */
   public static MappingTableCache getInstance()
   {
      return Registry.getSingleton(MappingTableCache.class);
   }

   private MappingTableCache(NXCSession session)
   {
      this.session = session;
      sessionListener = (n) -> {
         if ((n.getCode() == SessionNotification.MAPPING_TABLE_UPDATED)
               || (n.getCode() == SessionNotification.MAPPING_TABLE_DELETED))
         {
            invalidate((int)n.getSubCode());
         }
      };
      session.addListener(sessionListener);
   }

   /**
    * Resolve a raw value through the mapping table with the given ID. Returns the mapped string, or {@code null} when
    * there is no mapping configured ({@code id == 0}), the raw value is empty, the table is not yet cached (an async
    * load is scheduled), or the lookup misses. Callers should fall back to their default formatting on null.
    *
    * @param id mapping table ID; 0 means no mapping
    * @param rawValue raw collected value as string
    * @return mapped string, or {@code null} when no mapping applies
    */
   public String lookup(int id, String rawValue)
   {
      if (id == 0)
         return null;

      if ((rawValue == null) || rawValue.isEmpty())
         return null;

      MappingTable table = cache.get(id);
      if (table == null)
      {
         scheduleLoad(id);
         return null;
      }

      String mapped = table.lookup(rawValue);
      if (mapped == null)
         logger.debug("Mapping table {} ({}) has no entry for raw value \"{}\"", id, table.getName(), rawValue);
      return mapped;
   }

   /**
    * Drop cached entry for the given mapping table and notify listeners so any open view re-renders against the fresh
    * data on the next paint.
    *
    * @param id mapping table ID
    */
   public void invalidate(int id)
   {
      cache.remove(id);
      pending.remove(id);
      fireLoadListeners();
   }

   /**
    * Register a callback fired after a mapping table is loaded into the cache (or invalidated). Views displaying mapped
    * DCI values use this to re-render so a previously-missed mapping appears without waiting for the next auto-refresh
    * tick.
    *
    * <p>The listener is invoked from the loader / notification thread; listeners are responsible for posting to the UI
    * thread when needed.
    *
    * @param listener callback to register
    */
   public void addLoadListener(Runnable listener)
   {
      loadListeners.add(listener);
   }

   /**
    * Unregister a callback previously added via {@link #addLoadListener(Runnable)}.
    *
    * @param listener callback to remove
    */
   public void removeLoadListener(Runnable listener)
   {
      loadListeners.remove(listener);
   }

   /**
    * @see org.netxms.nxmc.DisposableSingleton#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(sessionListener);
      cache.clear();
      pending.clear();
      loadListeners.clear();
   }

   private void fireLoadListeners()
   {
      for(Runnable r : loadListeners)
      {
         try
         {
            r.run();
         }
         catch(Exception e)
         {
            logger.debug("Mapping table load listener threw", e);
         }
      }
   }

   private void scheduleLoad(final int id)
   {
      if (!pending.add(id))
         return;

      Thread loader = new Thread(() -> {
         boolean loaded = false;
         try
         {
            MappingTable table = session.getMappingTable(id);
            table.buildHash();
            cache.put(id, table);
            loaded = true;
            logger.debug("Loaded mapping table {} ({}) with {} entries", id, table.getName(), table.getData().size());
         }
         catch(Exception e)
         {
            logger.warn("Cannot load mapping table " + id, e);
         }
         finally
         {
            pending.remove(id);
            if (loaded)
               fireLoadListeners();
         }
      }, "MappingTableLoader-" + id);
      loader.setDaemon(true);
      loader.start();
   }
}
