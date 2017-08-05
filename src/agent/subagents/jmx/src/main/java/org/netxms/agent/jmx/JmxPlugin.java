/**
 * 
 */
package org.netxms.agent.jmx;

import java.util.HashMap;
import java.util.Map;
import javax.management.openmbean.CompositeData;
import org.netxms.agent.ListParameter;
import org.netxms.agent.Parameter;
import org.netxms.agent.ParameterType;
import org.netxms.agent.Plugin;
import org.netxms.agent.PluginInitException;
import org.netxms.agent.adapters.ListParameterAdapter;
import org.netxms.agent.adapters.ParameterAdapter;
import org.netxms.bridge.Config;
import org.netxms.bridge.ConfigEntry;
import org.netxms.bridge.Platform;

/**
 * JMX subagent plugin
 */
public class JmxPlugin extends Plugin
{
   private static final String OBJECT_CLASS_LOADING = "java.lang:type=ClassLoading";
   private static final String OBJECT_MEMORY = "java.lang:type=Memory";
   private static final String OBJECT_RUNTIME = "java.lang:type=Runtime";
   private static final String OBJECT_THREADING = "java.lang:type=Threading";
   
   private Map<String, Server> servers = new HashMap<String, Server>();
   
   /**
    * Constructor
    * 
    * @param config
    */
   public JmxPlugin(Config config)
   {
      super(config);
   }
   
   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#getName()
    */
   @Override
   public String getName()
   {
      return "JMX";
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#getVersion()
    */
   @Override
   public String getVersion()
   {
      return "2.1.1";
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#init(org.netxms.agent.Config)
    */
   @Override
   public void init(Config config) throws PluginInitException
   {
      super.init(config);
      
      ConfigEntry e = config.getEntry("/JMX/Server");
      if (e == null)
         throw new PluginInitException("JMX servers not defined");
         
      for(int i = 0; i < e.getValueCount(); i++)
      {
         addServer(e.getValue(i));
      }
   }
   
   /**
    * Add server from config
    * Format is
    *    name:url
    *    name:login@url
    *    name:login/password@url
    * 
    * @param config
    * @throws PluginInitException
    */
   private void addServer(String config) throws PluginInitException
   {
      String[] parts = config.split(":", 2);
      if (parts.length != 2)
         throw new PluginInitException("Invalid server configuration entry \"" + config + "\"");
      
      Server s;
      if (parts[1].contains("@"))
      {
         String[] uparts = parts[1].split("@", 2);
         String login;
         String password;
         if (uparts[0].contains("/"))
         {
            String[] lparts = uparts[0].split("/", 2);
            login = lparts[0];
            password = lparts[1];
         }
         else
         {
            login = uparts[0];
            password = "";
         }
         s = new Server(parts[0].trim(), uparts[1].trim(), login, password);
      }
      else
      {
         s = new Server(parts[0].trim(), parts[1].trim(), null, null);
      }
      servers.put(s.getName(), s);
      Platform.writeDebugLog(3, "JMX: added server connection " + s.getName() + " (" + s.getUrl() + ")");
   }
   
   /**
    * Get server by name
    * 
    * @param name
    * @return
    */
   private Server getServer(String name)
   {
      if ((name == null) || name.isEmpty())
         return null;
      return servers.get(name);
   }
   
   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#getParameters()
    */
   @Override
   public Parameter[] getParameters()
   {
      return new Parameter[] {
            new ParameterAdapter("JMX.ObjectAttribute(*)", "Value of JMX object attribute {instance}", ParameterType.STRING) {
               @Override
               public String getValue(String param) throws Exception
               {
                  Server server = getServer(getArgument(param, 0));
                  if (server == null)
                     return null;
                  
                  String object = getArgument(param, 1);
                  String attribute = getArgument(param, 2);
                  if ((object == null) || object.isEmpty() || (attribute == null) || attribute.isEmpty())
                     return null;
                  
                  String item = getArgument(param, 3);
                  if ((item == null) || item.isEmpty())
                     return server.getAttributeValueAsString(object, attribute);
                  
                  Object value = server.getAttributeValue(object, attribute);
                  if (!(value instanceof CompositeData))
                     return null;                  
                  return ((CompositeData)value).get(item).toString();
               }
            },
            new JmxObjectParameterAdapter("JMX.Memory.ObjectsPendingFinalization(*)", "JVM {instance}: objects pending finalization", OBJECT_MEMORY, "ObjectPendingFinalizationCount", ParameterType.UINT),
            new JmxObjectParameterAdapter("JMX.Memory.Heap.Committed(*)", "JVM {instance}: committed heap memory", OBJECT_MEMORY, "HeapMemoryUsage", "committed", ParameterType.UINT64),
            new JmxObjectParameterAdapter("JMX.Memory.Heap.Current(*)", "JVM {instance}: current heap size", OBJECT_MEMORY, "HeapMemoryUsage", "used", ParameterType.UINT64),
            new JmxObjectParameterAdapter("JMX.Memory.Heap.Init(*)", "JVM {instance}: initial heap size", OBJECT_MEMORY, "HeapMemoryUsage", "init", ParameterType.UINT64),
            new JmxObjectParameterAdapter("JMX.Memory.Heap.Max(*)", "JVM {instance}: maximum heap size", OBJECT_MEMORY, "HeapMemoryUsage", "max", ParameterType.UINT64),
            new JmxObjectParameterAdapter("JMX.Memory.NonHeap.Committed(*)", "JVM {instance}: committed non-heap memory", OBJECT_MEMORY, "NonHeapMemoryUsage", "committed", ParameterType.UINT64),
            new JmxObjectParameterAdapter("JMX.Memory.NonHeap.Current(*)", "JVM {instance}: current non-heap memory size", OBJECT_MEMORY, "NonHeapMemoryUsage", "used", ParameterType.UINT64),
            new JmxObjectParameterAdapter("JMX.Memory.NonHeap.Init(*)", "JVM {instance}: initial non-heap memory size", OBJECT_MEMORY, "NonHeapMemoryUsage", "init", ParameterType.UINT64),
            new JmxObjectParameterAdapter("JMX.Memory.NonHeap.Max(*)", "JVM {instance}: maximum non-heap memory size", OBJECT_MEMORY, "NonHeapMemoryUsage", "max", ParameterType.UINT64),
            new JmxObjectParameterAdapter("JMX.Threads.Count(*)", "JVM {instance}: live threads", OBJECT_THREADING, "ThreadCount", ParameterType.UINT),
            new JmxObjectParameterAdapter("JMX.Threads.DaemonCount(*)", "JVM {instance}: daemon threads", OBJECT_THREADING, "DaemonThreadCount", ParameterType.UINT),
            new JmxObjectParameterAdapter("JMX.Threads.PeakCount(*)", "JVM {instance}: peak number of threads", OBJECT_THREADING, "PeakThreadCount", ParameterType.UINT),
            new JmxObjectParameterAdapter("JMX.Threads.TotalStarted(*)", "JVM {instance}: total threads started", OBJECT_THREADING, "TotalStartedThreadCount", ParameterType.UINT),
            new JmxObjectParameterAdapter("JMX.VM.BootClassPath(*)", "JVM {instance}: boot class path", OBJECT_RUNTIME, "BootClassPath", ParameterType.STRING),
            new JmxObjectParameterAdapter("JMX.VM.ClassPath(*)", "JVM {instance}: class path", OBJECT_RUNTIME, "ClassPath", ParameterType.STRING),
            new JmxObjectParameterAdapter("JMX.VM.LoadedClassCount(*)", "JVM {instance}: loaded class count", OBJECT_CLASS_LOADING, "LoadedClassCount", ParameterType.UINT),
            new JmxObjectParameterAdapter("JMX.VM.Name(*)", "JVM {instance}: name", OBJECT_RUNTIME, "VmName", ParameterType.STRING),
            new JmxObjectParameterAdapter("JMX.VM.SpecVersion(*)", "JVM {instance}: specification version", OBJECT_RUNTIME, "SpecVersion", ParameterType.STRING),
            new JmxObjectParameterAdapter("JMX.VM.TotalLoadedClassCount(*)", "JVM {instance}: total loaded class count", OBJECT_CLASS_LOADING, "TotalLoadedClassCount", ParameterType.UINT),
            new JmxObjectParameterAdapter("JMX.VM.UnloadedClassCount(*)", "JVM {instance}: unloaded class count", OBJECT_CLASS_LOADING, "UnloadedClassCount", ParameterType.UINT),
            new JmxObjectParameterAdapter("JMX.VM.Uptime(*)", "JVM {instance}: uptime", OBJECT_RUNTIME, "Uptime", ParameterType.UINT),
            new JmxObjectParameterAdapter("JMX.VM.Vendor(*)", "JVM {instance}: vendor", OBJECT_RUNTIME, "VmVendor", ParameterType.STRING),
            new JmxObjectParameterAdapter("JMX.VM.Version(*)", "JVM {instance}: version", OBJECT_RUNTIME, "VmVersion", ParameterType.STRING),
      };
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#getListParameters()
    */
   @Override
   public ListParameter[] getListParameters()
   {
      return new ListParameter[] {
            new ListParameterAdapter("JMX.Domains(*)", "Available JMX domains") {
               @Override
               public String[] getValue(String param) throws Exception
               {
                  Server server = getServer(getArgument(param, 0));
                  if (server == null)
                     return null;
                  return server.getDomains();
               }
            },
            new ListParameterAdapter("JMX.ObjectAttributes(*)", "Available JMX object attributes") {
               @Override
               public String[] getValue(String param) throws Exception
               {
                  Server server = getServer(getArgument(param, 0));
                  if (server == null)
                     return null;
                  return server.getObjectAttributes(getArgument(param, 1));
               }
            },
            new ListParameterAdapter("JMX.Objects(*)", "Available JMX objects") {
               @Override
               public String[] getValue(String param) throws Exception
               {
                  Server server = getServer(getArgument(param, 0));
                  if (server == null)
                     return null;
                  return server.getObjects(getArgument(param, 1));
               }
            }
      };
   }

   /**
    * Adapter for JVM parameters
    */
   public class JmxObjectParameterAdapter extends ParameterAdapter
   {
      private String object;
      private String attribute;
      private String item;
      
      public JmxObjectParameterAdapter(String name, String description, String object, String attribute, ParameterType type)
      {
         this(name, description, object, attribute, null, type);
      }

      public JmxObjectParameterAdapter(String name, String description, String object, String attribute, String item, ParameterType type)
      {
         super(name, description, type);
         this.object = object;
         this.attribute = attribute;
         this.item = item;
      }

      /* (non-Javadoc)
       * @see org.netxms.agent.Parameter#getValue(java.lang.String)
       */
      @Override
      public String getValue(String param) throws Exception
      {
         Server server = getServer(getArgument(param, 0));
         if (server == null)
            return null;
         if (item == null)
            return server.getAttributeValueAsString(object, attribute);
         Object value = server.getAttributeValue(object, attribute);
         if (!(value instanceof CompositeData))
            return null;
         return ((CompositeData)value).get(item).toString();
      }
   }
}
