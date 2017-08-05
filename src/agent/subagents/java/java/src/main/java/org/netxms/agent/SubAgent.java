/**
 * Java-Bridge NetXMS subagent
 * Copyright (C) 2013 TEMPEST a.s.
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
package org.netxms.agent;

import java.io.File;
import java.lang.reflect.Constructor;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.jar.Attributes;
import java.util.jar.Manifest;
import org.netxms.bridge.Config;
import org.netxms.bridge.ConfigEntry;
import org.netxms.bridge.DirectoryType;
import org.netxms.bridge.LogLevel;
import org.netxms.bridge.Platform;

/**
 * This class is a Java representation of native subagent exposing to it required APIs
 * It also exposes native sendTrap, pushData and writeMessage agent API
 */
public class SubAgent
{
   private static final String JAR = ".jar";
   private static final String PLUGIN_CLASSNAME_ATTRIBUTE_NAME = "NetXMS-Plugin-Classname";
   private static final String MANIFEST_PATH = "META-INF/MANIFEST.MF";

   protected Map<String, Plugin> plugins;
   protected Map<String, Action> actions;
   protected Map<String, Parameter> parameters;
   protected Map<String, ListParameter> lists;
   protected Map<String, PushParameter> pushParameters;
   protected Map<String, TableParameter> tables;

   private Config config = null;

   /**
    * Private constructor. Will be invoked by native wrapper only.
    * 
    * @param config agent configuration
    */
   private SubAgent(Config config)
   {
      plugins = new HashMap<String, Plugin>();
      actions = new HashMap<String, Action>();
      parameters = new HashMap<String, Parameter>();
      lists = new HashMap<String, ListParameter>();
      pushParameters = new HashMap<String, PushParameter>();
      tables = new HashMap<String, TableParameter>();

      this.config = config;
      Platform.writeDebugLog(1, "Java SubAgent created");

      // load all Plugins
      ConfigEntry configEntry = config.getEntry("/Java/Plugin");
      if (configEntry != null)
      {
         for(int i = 0; i < configEntry.getValueCount(); i++)
         {
            String entry = configEntry.getValue(i).trim();
            try
            {
               loadPlugin(entry);
            }
            catch(Throwable e)
            {
               Platform.writeLog(LogLevel.ERROR, "JAVA: Exception in loadPlugin: " + e.getClass().getCanonicalName() + ": " + e.getMessage());
               Platform.writeDebugLog(6, "JAVA:   ", e);
            }
         }
      }
      else
      {
         Platform.writeLog(LogLevel.WARNING, "No plugins defined in Java section");
      }
   }

   /**
    * Get agent configuration object
    * 
    * @return agent configuration
    */
   protected Config getConfig()
   {
      return config;
   }

   /*===== native methods exposed by agent (see nms_agent.h) =====*/
   
   /**
    * Extract argument from full parameter name.
    * 
    * For example:
    *   parameter: Process.Count(java, 2)
    *   
    *   AgentGetParameterArg(parameter, 1) will return "java"
    * 
    * @param param full parameter name (as passed to handler)
    * @param index argument index (first argument has index 1)
    * @return extracted argument on success, empty string if argument is missing, null if input is malformed
    */
   public static native String getParameterArg(String param, int index);

   public static native void sendTrap(int event, String name, String[] args);

   protected static native boolean pushParameterData(String name, String value);

   /*===== end of native methods exposed by agent =====*/

   /**
    * Initialize (to be called from native subagent)
    * 
    * @param config agent configuration
    * @return true if initialization was successful
    */
   public boolean init()
   {
      Platform.writeDebugLog(2, "JAVA: subagent initialization started");
      for(Map.Entry<String, Plugin> entry : plugins.entrySet())
      {
         try
         {
            Platform.writeDebugLog(5, "JAVA: calling init() method for plugin " + entry.getKey());
            entry.getValue().init(config);
         }
         catch(Throwable e)
         {
            Platform.writeDebugLog(2, "JAVA: exception in plugin " + entry.getKey() + " initialization handler: " + e.getClass().getCanonicalName() + ": " + e.getMessage());
            Platform.writeDebugLog(6, "JAVA:   ", e);
         }
      }
      Platform.writeDebugLog(2, "JAVA: subagent initialization completed");
      return true;
   }

   /**
    * Shutdown (to be called from native subagent)
    */
   public void shutdown()
   {
      Platform.writeDebugLog(2, "JAVA: subagent shutdown initiated");
      for(Map.Entry<String, Plugin> entry : plugins.entrySet())
      {
         try
         {
            Platform.writeDebugLog(5, "JAVA: calling shutdown() method for plugin " + entry.getKey());
            entry.getValue().shutdown();
         }
         catch(Throwable e)
         {
            Platform.writeDebugLog(2, "JAVA: exception in plugin " + entry.getKey() + " shutdown handler: " + e.getClass().getCanonicalName() + ": " + e.getMessage());
            Platform.writeDebugLog(6, "JAVA:   ", e);
         }
      }
      Platform.writeDebugLog(2, "JAVA: subagent shutdown completed");
   }

   /**
    * will be called from subagent native initialization
    * 
    * @param path path to plugin file
    * @return true if plugin was loaded correctly
    */
   protected boolean loadPlugin(String path)
   {
      Platform.writeDebugLog(2, "SubAgent.loadPlugin(" + path + ")");
      Plugin[] loadList = path.toLowerCase().endsWith(JAR) ? createPluginWithJar(path, config) : createPluginWithClassname(path, config);
      
      if ((loadList == null) || (loadList.length == 0))
         return false;
      
      for(Plugin p : loadList)
      {
         plugins.put(p.getName(), p);
         Platform.writeDebugLog(2, "Java plugin " + p.getName() + " (" + p.getClass().getName() + ") loaded");
         
         // register actions
         Action[] _actions = p.getActions();
         for(int i = 0; i < _actions.length; i++)
            actions.put(createContributionItemId(p, _actions[i]), _actions[i]);
      
         // register paramaters
         Parameter[] _parameters = p.getParameters();
         for(int i = 0; i < _parameters.length; i++)
            parameters.put(createContributionItemId(p, _parameters[i]), _parameters[i]);
      
         // register list paramaters
         ListParameter[] _listParameters = p.getListParameters();
         for(int i = 0; i < _listParameters.length; i++)
            lists.put(createContributionItemId(p, _listParameters[i]), _listParameters[i]);
      
         // register push paramaters
         PushParameter[] _pushParameters = p.getPushParameters();
         for(int i = 0; i < _pushParameters.length; i++)
            pushParameters.put(createContributionItemId(p, _pushParameters[i]), _pushParameters[i]);
      
         // register table paramaters
         TableParameter[] _tableParameters = p.getTableParameters();
         for(int i = 0; i < _tableParameters.length; i++)
            tables.put(createContributionItemId(p, _tableParameters[i]), _tableParameters[i]);
      
         Platform.writeDebugLog(8, "SubAgent.loadPlugin actions=" + actions);
         Platform.writeDebugLog(8, "SubAgent.loadPlugin parameters=" + parameters);
         Platform.writeDebugLog(8, "SubAgent.loadPlugin listParameters=" + lists);
         Platform.writeDebugLog(8, "SubAgent.loadPlugin pushParameters=" + pushParameters);
         Platform.writeDebugLog(8, "SubAgent.loadPlugin tableParameters=" + tables);
      }
      return true;
   }

   /**
    * Create plugin with given class name.
    * 
    * @param classname plugin class name
    * @param config agent configuration
    * @return plugin object or null
    */
   protected Plugin[] createPluginWithClassname(String classname, Config config)
   {
      try
      {
         Class<?> pluginClass = Class.forName(classname);
         Platform.writeDebugLog(3, "SubAgent.createPluginWithClassname loaded class " + pluginClass);
         Plugin plugin = instantiatePlugin(pluginClass.asSubclass(Plugin.class), config);
         Platform.writeDebugLog(3, "SubAgent.createPluginWithClassname created instance " + plugin);
         return new Plugin[] { plugin };
      }
      catch(Throwable e)
      {
         Platform.writeLog(LogLevel.WARNING, "Failed to load plugin " + classname + ": " + e.getClass().getCanonicalName() + ": " + e.getMessage());
         Platform.writeDebugLog(6, "JAVA:   ", e);
         return null;
      }
   }

   /**
    * Create plugins from given jar file
    * 
    * @param jarFile jar file to load
    * @param config agent configuration
    * @return loaded plugins or null
    */
   protected Plugin[] createPluginWithJar(String jarFile, Config config)
   {
      String classList;
      URLClassLoader classLoader;
      try
      {
         File file = new File(jarFile);
         if (!file.isAbsolute())
         {
            Platform.writeDebugLog(5, "Plugin file path \"" + jarFile + "\" is not absolute");
            String libDir = Platform.getNetXMSDirectory(DirectoryType.LIB);
            if ((libDir != null) && !libDir.isEmpty())
            {
               file = new File(libDir + File.separatorChar + jarFile);
            }
         }
         
         if (!file.isFile())
         {
            Platform.writeLog(LogLevel.WARNING, "File \"" + file.getAbsolutePath() + "\" does not exist or is not a regular file");
            return null;
         }
         
         Platform.writeDebugLog(2, "Loading plugin file \"" + file.getAbsolutePath() + "\"");
         classLoader = new URLClassLoader(new URL[] { file.toURI().toURL() }, Thread.currentThread().getContextClassLoader() );
         URL url = classLoader.findResource(MANIFEST_PATH);
         if (url != null)
         {
            Manifest manifest = new Manifest(url.openStream());
            Attributes attributes = manifest.getMainAttributes();
            classList = attributes.getValue(PLUGIN_CLASSNAME_ATTRIBUTE_NAME);
            if (classList == null)
            {
               classLoader.close();
               Platform.writeLog(LogLevel.WARNING, "Failed to find " + PLUGIN_CLASSNAME_ATTRIBUTE_NAME + " attribute in manifest of " + jarFile);
               return null;
            }
         }
         else
         {
            classLoader.close();
            Platform.writeLog(LogLevel.WARNING, "Error processing jar file " + jarFile + ": manifest not found");
            return null;
         }
      }
      catch(Throwable e)
      {
         Platform.writeLog(LogLevel.WARNING, "Error processing jar file " + jarFile + ": " + e.getClass().getCanonicalName() + ": " + e.getMessage());
         Platform.writeDebugLog(6, "JAVA:   ", e);
         return null;
      }
         
      List<Plugin> pluginList = new ArrayList<Plugin>();
      String[] classNames = classList.split(";");
      for(String cn : classNames)
      {
         cn = cn.trim();
         try
         {
            Class<?> pluginClass = Class.forName(cn, false, classLoader);
            Plugin plugin = instantiatePlugin(pluginClass.asSubclass(Plugin.class), config);
            pluginList.add(plugin);
         }
         catch(Throwable e)
         {
            Platform.writeLog(LogLevel.WARNING, "Failed to load plugin " + cn + " from jar file " + jarFile + ": " + e.getClass().getCanonicalName() + ": " + e.getMessage());
            Platform.writeDebugLog(6, "JAVA:   ", e);
         }
      }
      return pluginList.toArray(new Plugin[pluginList.size()]);
   }

   /**
    * uses reflection to create an instance of Plugin
    * 
    * @param pluginClass plugin class
    * @param config agent configuration
    * @return plugin object
    * @throws Exception
    */
   protected Plugin instantiatePlugin(Class<? extends Plugin> pluginClass, Config config) throws Exception
   {
      Plugin plugin = null;
      Constructor<? extends Plugin> constructor = pluginClass.getDeclaredConstructor(Config.class);
      if (constructor != null)
      {
         plugin = constructor.newInstance(config);
      }
      else
      {
         throw new Exception("Failed to find constructor Plugin(Config) in class " + pluginClass);
      }
      return plugin;
   }

   /**
    * to be called from native subagent
    * 
    * @param param
    * @param id
    * @return
    * @throws Throwable
    */
   public String parameterHandler(final String param, final String id) throws Throwable
   {
      try
      {
         Parameter parameter = parameters.get(id);
         if (parameter != null)
         {
            return parameter.getValue(param);
         }
         return null;
      }
      catch(Throwable e)
      {
         Platform.writeDebugLog(6, "JAVA: Exception in parameter handler: " + e.getClass().getCanonicalName() + ": " + e.getMessage());
         Platform.writeDebugLog(6, "JAVA:   ", e);
         throw e;
      }
   }

   /**
    * to be called from native subagent
    * 
    * @param param
    * @param id
    * @return
    * @throws Throwable
    */
   public String[] listHandler(final String param, final String id) throws Throwable
   {
      try
      {
         ListParameter listParameter = lists.get(id);
         if (listParameter != null)
         {
            return listParameter.getValue(param);
         }
         return null;
      }
      catch(Throwable e)
      {
         Platform.writeDebugLog(6, "JAVA: Exception in list handler: " + e.getClass().getCanonicalName() + ": " + e.getMessage());
         Platform.writeDebugLog(6, "JAVA:   ", e);
         throw e;
      }
   }

   /**
    * to be called from native subagent
    * 
    * @param param
    * @param id
    * @return
    * @throws Throwable
    */
   public String[][] tableHandler(final String param, final String id) throws Throwable
   {
      try
      {
         TableParameter tableParameter = tables.get(id);
         if (tableParameter != null)
         {
            Platform.writeDebugLog(7, "SubAgent.tableParameterHandler(param=" + param + ", id=" + id + ") returning " + tableParameter.getValue(param));
            return tableParameter.getValue(param);
         }
         return null;
      }
      catch(Throwable e)
      {
         Platform.writeDebugLog(6, "JAVA: Exception in table handler: " + e.getClass().getCanonicalName() + ": " + e.getMessage());
         Platform.writeDebugLog(6, "JAVA:   ", e);
         throw e;
      }
   }

   /**
    * to be called from native subagent
    * 
    * @param name action name
    * @param args action arguments
    * @param id action ID
    * @return true if executed successfully
    */
   public boolean actionHandler(final String name, final String[] args, final String id)
   {
      try
      {
         Action action = actions.get(id);
         if (action != null)
         {
            return action.execute(name, args);
         }
      }
      catch(Throwable e)
      {
         Platform.writeDebugLog(6, "JAVA: Exception in action handler: " + e.getClass().getCanonicalName() + ": " + e.getMessage());
         Platform.writeDebugLog(6, "JAVA:   ", e);
      }
      return false;
   }

   /**
    * @return
    */
   public String[] getActions()
   {
      List<String> list = new ArrayList<String>();
      for(Entry<String, Action> e : actions.entrySet())
      {
         list.add(e.getKey());
         list.add(e.getValue().getName());
         list.add(e.getValue().getDescription());
      }
      return list.toArray(new String[0]);
   }

   /**
    * @return
    */
   public String[] getParameters()
   {
      List<String> list = new ArrayList<String>();
      for(Entry<String, Parameter> e : parameters.entrySet())
      {
         list.add(e.getKey());
         list.add(e.getValue().getName());
         list.add(e.getValue().getDescription());
         list.add(Integer.toString(e.getValue().getType().getValue()));
      }
      return list.toArray(new String[0]);
   }

   /**
    * @return
    */
   public String[] getLists()
   {
      List<String> list = new ArrayList<String>();
      for(Entry<String, ListParameter> e : lists.entrySet())
      {
         list.add(e.getKey());
         list.add(e.getValue().getName());
         list.add(e.getValue().getDescription());
      }
      return list.toArray(new String[0]);
   }

   /**
    * @return
    */
   public String[] getPushParameters()
   {
      List<String> list = new ArrayList<String>();
      for(Entry<String, PushParameter> e : pushParameters.entrySet())
      {
         list.add(e.getKey());
         list.add(e.getValue().getName());
         list.add(e.getValue().getDescription());
         list.add(Integer.toString(e.getValue().getType().getValue()));
      }
      return list.toArray(new String[0]);
   }

   /**
    * @return
    */
   public String[] getTables()
   {
      List<String> list = new ArrayList<String>();
      if (tables.size() > 0)
      {
         list.add(Integer.toString(tables.size()));
         for(Entry<String, TableParameter> e : tables.entrySet())
         {
            list.add(e.getKey());
            list.add(e.getValue().getName());
            list.add(e.getValue().getDescription());
            
            TableColumn[] columns = e.getValue().getColumns();
            list.add(Integer.toString(columns.length));
            for(TableColumn tc : columns)
            {
               list.add(tc.getName());
               list.add(tc.getDisplayName());
               list.add(Integer.toString(tc.getType().getValue()));
               list.add(tc.isInstance() ? "T" : "F");
            }
         }
      }
      return list.toArray(new String[0]);
   }
   
   /**
    * Create contribution item ID.
    * 
    * @param plugin plugin object
    * @param ci contribution item
    * @return contribution item ID
    */
   private static String createContributionItemId(Plugin plugin, AgentContributionItem ci)
   {
      return plugin.getName() + "/" + ci.getName();
   }
}
