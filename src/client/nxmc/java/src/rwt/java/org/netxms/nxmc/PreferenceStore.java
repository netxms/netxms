/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashSet;
import java.util.ServiceLoader;
import java.util.Set;
import java.util.UUID;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.util.IPropertyChangeListener;
import org.eclipse.jface.util.PropertyChangeEvent;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.internal.service.ContextProvider;
import org.eclipse.rap.rwt.service.UISession;
import org.eclipse.swt.widgets.Display;
import org.netxms.nxmc.services.PreferenceInitializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import jakarta.servlet.http.Cookie;

/**
 * Local preference store
 */
public class PreferenceStore extends Memento implements IPreferenceStore
{
   private static final Logger logger = LoggerFactory.getLogger(PreferenceStore.class);
   private static final String COOKIE_NAME = "nxmcStoreId";
   private static final int COOKIE_MAX_AGE_SEC = 3600 * 24 * 90; // 3 months

   /**
    * Open local store
    */
   protected static void open(String stateDir)
   {
      PreferenceStore instance = new PreferenceStore(new File(stateDir + File.separator + "nxmc.preferences." + getStoreId()));
      ServiceLoader<PreferenceInitializer> loader = ServiceLoader.load(PreferenceInitializer.class, PreferenceStore.class.getClassLoader());
      for(PreferenceInitializer pi : loader)
      {
         logger.debug("Calling preference initializer " + pi.toString());
         try
         {
            pi.initializeDefaultPreferences(instance);
         }
         catch(Exception e)
         {
            logger.error("Exception in preference initializer", e);
         }
      }
      RWT.getUISession().setAttribute("netxms.preferenceStore", instance);
   }

   /**
    * Get store ID from session or cookie
    *
    * @return store ID from session or cookie
    */
   private static String getStoreId()
   {
      UISession uiSession = ContextProvider.getUISession();
      String result = (String)uiSession.getAttribute(COOKIE_NAME);
      if (result == null)
      {
         result = getStoreIdFromCookie();
         if (result == null)
         {
            result = UUID.randomUUID().toString();
         }
         Cookie cookie = new Cookie(COOKIE_NAME, result);
         cookie.setSecure(ContextProvider.getRequest().isSecure());
         cookie.setMaxAge(COOKIE_MAX_AGE_SEC);
         cookie.setHttpOnly(true);
         ContextProvider.getResponse().addCookie(cookie);
         uiSession.setAttribute(COOKIE_NAME, result);
      }
      return result;
   }

   /**
    * Get store ID from cookie
    *
    * @return store ID or null
    */
   private static String getStoreIdFromCookie()
   {
      String result = null;
      Cookie[] cookies = ContextProvider.getRequest().getCookies();
      if (cookies != null)
      {
         for(int i = 0; result == null && i < cookies.length; i++)
         {
            Cookie cookie = cookies[i];
            if (COOKIE_NAME.equals(cookie.getName()))
            {
               String value = cookie.getValue();
               // Validate cookies to prevent cookie manipulation and related attacks
               // see https://bugs.eclipse.org/bugs/show_bug.cgi?id=275380
               try
               {
                  UUID.fromString(value);
                  result = value;
               }
               catch(IllegalArgumentException e)
               {
                  logger.warn("Invalid store ID \"" + value + "\" received from client");
               }
            }
         }
      }
      return result;
   }

   /**
    * Get instance of preference store
    * 
    * @return instance of preference store
    */
   public static PreferenceStore getInstance()
   {
      return (PreferenceStore)RWT.getUISession().getAttribute("netxms.preferenceStore");
   }

   /**
    * Get instance of preference store
    * 
    * @param display display to return store for
    * @return instance of preference store
    */
   public static PreferenceStore getInstance(Display display)
   {
      return (PreferenceStore)RWT.getUISession(display).getAttribute("netxms.preferenceStore");
   }

   private File storeFile;
   private Set<IPropertyChangeListener> changeListeners = new HashSet<IPropertyChangeListener>();

   /**
    * Default constructor
    */
   private PreferenceStore(File storeFile)
   {
      super();
      this.storeFile = storeFile;
      if (storeFile.exists())
      {
         FileReader reader = null;
         try
         {
            reader = new FileReader(storeFile);
            properties.load(reader);
         }
         catch(Exception e)
         {
            logger.error("Error reading local preferences from " + storeFile.getAbsolutePath(), e);
         }
         finally
         {
            if (reader != null)
            {
               try
               {
                  reader.close();
               }
               catch(IOException e)
               {
               }
            }
         }
      }
   }

   /**
    * Save preference store
    */
   private void save()
   {
      FileWriter writer = null;
      try
      {
         writer = new FileWriter(storeFile);
         properties.store(writer, "NXMC local preferences");
      }
      catch(Exception e)
      {
         logger.error("Error writing local preferences to " + storeFile.getAbsolutePath(), e);
      }
      finally
      {
         if (writer != null)
         {
            try
            {
               writer.close();
            }
            catch(IOException e)
            {
            }
         }
      }
   }

   /**
    * Add property change listener.
    *
    * @param listener listener to add
    */
   public void addPropertyChangeListener(IPropertyChangeListener listener)
   {
      synchronized(changeListeners)
      {
         changeListeners.add(listener);
      }
   }

   /**
    * Remove property change listener.
    *
    * @param listener listener to remove
    */
   public void removePropertyChangeListener(IPropertyChangeListener listener)
   {
      synchronized(changeListeners)
      {
         changeListeners.remove(listener);
      }
   }

   /**
    * @see org.netxms.nxmc.Memento#onPropertyChange(java.lang.String, java.lang.String, java.lang.String)
    */
   @Override
   protected void onPropertyChange(String property, String oldValue, String newValue)
   {
      PropertyChangeEvent event = new PropertyChangeEvent(this, property, oldValue, newValue);
      synchronized(changeListeners)
      {
         for(IPropertyChangeListener l : changeListeners)
            l.propertyChange(event);
      }
      save();
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#contains(java.lang.String)
    */
   @Override
   public boolean contains(String name)
   {
      return properties.contains(name);
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#firePropertyChangeEvent(java.lang.String, java.lang.Object, java.lang.Object)
    */
   @Override
   public void firePropertyChangeEvent(String name, Object oldValue, Object newValue)
   {
      onPropertyChange(name, (String)oldValue, (String)newValue);
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#getBoolean(java.lang.String)
    */
   @Override
   public boolean getBoolean(String name)
   {
      return getAsBoolean(name, false);
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#getDefaultBoolean(java.lang.String)
    */
   @Override
   public boolean getDefaultBoolean(String name)
   {
      String v = defaultValues.getProperty(name);
      return (v != null) ? Boolean.parseBoolean(v) : false;
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#getDefaultDouble(java.lang.String)
    */
   @Override
   public double getDefaultDouble(String name)
   {
      String v = defaultValues.getProperty(name);
      if (v == null)
         return DOUBLE_DEFAULT_DEFAULT;
      try
      {
         return Double.parseDouble(v);
      }
      catch(NumberFormatException e)
      {
         return DOUBLE_DEFAULT_DEFAULT;
      }
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#getDefaultFloat(java.lang.String)
    */
   @Override
   public float getDefaultFloat(String name)
   {
      String v = defaultValues.getProperty(name);
      if (v == null)
         return FLOAT_DEFAULT_DEFAULT;
      try
      {
         return Float.parseFloat(v);
      }
      catch(NumberFormatException e)
      {
         return FLOAT_DEFAULT_DEFAULT;
      }
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#getDefaultInt(java.lang.String)
    */
   @Override
   public int getDefaultInt(String name)
   {
      String v = defaultValues.getProperty(name);
      if (v == null)
         return INT_DEFAULT_DEFAULT;
      try
      {
         return Integer.parseInt(v);
      }
      catch(NumberFormatException e)
      {
         return INT_DEFAULT_DEFAULT;
      }
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#getDefaultLong(java.lang.String)
    */
   @Override
   public long getDefaultLong(String name)
   {
      String v = defaultValues.getProperty(name);
      if (v == null)
         return LONG_DEFAULT_DEFAULT;
      try
      {
         return Long.parseLong(v);
      }
      catch(NumberFormatException e)
      {
         return LONG_DEFAULT_DEFAULT;
      }
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#getDefaultString(java.lang.String)
    */
   @Override
   public String getDefaultString(String name)
   {
      String v = defaultValues.getProperty(name);
      if (v == null)
         return "";
      return v;
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#getDouble(java.lang.String)
    */
   @Override
   public double getDouble(String name)
   {
      return getAsDouble(name, DOUBLE_DEFAULT_DEFAULT);
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#getFloat(java.lang.String)
    */
   @Override
   public float getFloat(String name)
   {
      String v = properties.getProperty(name);
      if (v == null)
         return getDefaultFloat(name);
      try
      {
         return Float.parseFloat(v);
      }
      catch(NumberFormatException e)
      {
         return getDefaultFloat(name);
      }
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#getInt(java.lang.String)
    */
   @Override
   public int getInt(String name)
   {
      return getAsInteger(name, getDefaultInt(name));
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#getLong(java.lang.String)
    */
   @Override
   public long getLong(String name)
   {
      return getAsLong(name, getDefaultLong(name));
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#getString(java.lang.String)
    */
   @Override
   public String getString(String name)
   {
      String value = getAsString(name);
      return value == null ? "" : value;
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#isDefault(java.lang.String)
    */
   @Override
   public boolean isDefault(String name)
   {
      String v = defaultValues.getProperty(name);
      return v != null;
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#needsSaving()
    */
   @Override
   public boolean needsSaving()
   {
      return false;
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#putValue(java.lang.String, java.lang.String)
    */
   @Override
   public void putValue(String name, String value)
   {
      set(name, value);      
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#setDefault(java.lang.String, double)
    */
   @Override
   public void setDefault(String name, double value)
   {
      setDefault(name, value);
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#setDefault(java.lang.String, float)
    */
   @Override
   public void setDefault(String name, float value)
   {
      setDefault(name, value);      
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#setToDefault(java.lang.String)
    */
   @Override
   public void setToDefault(String name)
   {
      set(name, getDefaultString(name));
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#setValue(java.lang.String, double)
    */
   @Override
   public void setValue(String name, double value)
   {
      set(name, value);       
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#setValue(java.lang.String, float)
    */
   @Override
   public void setValue(String name, float value)
   {
      set(name, value); 
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#setValue(java.lang.String, int)
    */
   @Override
   public void setValue(String name, int value)
   {
      set(name, value); 
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#setValue(java.lang.String, long)
    */
   @Override
   public void setValue(String name, long value)
   {
      set(name, value); 
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#setValue(java.lang.String, java.lang.String)
    */
   @Override
   public void setValue(String name, String value)
   {
      set(name, value); 
   }

   /**
    * @see org.eclipse.jface.preference.IPreferenceStore#setValue(java.lang.String, boolean)
    */
   @Override
   public void setValue(String name, boolean value)
   {
      set(name, value); 
   }
}
