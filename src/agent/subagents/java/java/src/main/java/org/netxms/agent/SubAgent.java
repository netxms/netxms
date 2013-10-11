package org.netxms.agent;

// This class is a Java representation of native subagent exposing to it required APIs
// It also exposes native sendTrap, pushData and writeMessage agent API

import java.lang.reflect.Constructor;
import java.util.HashMap;
import java.util.Map;
import java.util.jar.Manifest;
import java.util.jar.Attributes;
import java.net.URL;
import java.net.URLClassLoader;
import bsh.*;

public class SubAgent {

	public enum LogLevel {
		DEBUG(0x0080),
		INFO(0x0004),
		WARNING(0x0002),
		ERROR(0x0001);

		private int value;

		LogLevel(final int value) {
		    this.value = value;
		}

		public int getValue() {
		    return value;
		 }
	}

	private static final String JAR = ".jar";
	public static final String PLUGIN_CLASSNAME_ATTRIBUTE_NAME = "NetXMS-Plugin-Classname";
	public static final String MANIFEST_PATH = "META-INF/MANIFEST.MF";
	
	protected Map<String, Plugin> plugins;
	protected Map<String, Action> actions;
	protected Map<String, Parameter> parameters;
	protected Map<String, ListParameter> listParameters;
	protected Map<String, PushParameter> pushParameters;
	protected Map<String, TableParameter> tableParameters;
	
	protected Config getConfig() {
		return config;
	}
	
	private Config config = null;

	// native methods exposed by agent (see nms_agent.h)
	protected static native String AgentGetParameterArg(String param, int index);
	protected static native void AgentSendTrap(int event, String name, String[]args);
	protected static native boolean AgentPushParameterData(String name, String value);
	
	protected static native void AgentWriteLog(int level, String message);
	protected static void AgentWriteLog(LogLevel level, String message) {
		AgentWriteLog(level.getValue(), message);
	}

	protected static native void AgentWriteDebugLog(int level, String message);
	protected static void AgentWriteDebugLog(LogLevel level, String message) {
		AgentWriteDebugLog(level.getValue(), message);
	}
	// end of native methods exposed by agent
	
	// to be called from native subagent
	public boolean init(Config config) {
		for (Map.Entry<String, Plugin> entry: plugins.entrySet()) {
			entry.getValue().init(config);
		}
		return true;
	}
	
	// to be called from native subagent
	public void shutdown() {
		AgentWriteLog(LogLevel.DEBUG, "SubAgent.shutdown()");
		for (Map.Entry<String, Plugin> entry: plugins.entrySet()) {
			entry.getValue().shutdown();
		}
	}

//	public boolean commandHandler() {
//		return true;
//	}

	// will be called from subagent native initialization
	protected boolean loadPlugin(String path, Config config) {
		AgentWriteLog(LogLevel.DEBUG, "SubAgent.loadPlugin(" + path + ")");
		Plugin plugin = null;
		try {
			plugin = path.toLowerCase().endsWith(JAR) ? createPluginWithJar(path, config) : createPluginWithClassname(path, config);
			// TODO need to set it up with at least Config
		} catch (Exception ex) {
			AgentWriteLog(LogLevel.WARNING, "Failed to load plugin " + path + " with error: " + ex);
		} finally {
			if (plugin != null) {
				plugins.put(plugin.getName(), plugin);
				// register actions
				Action[] _actions = plugin.getActions();
				for (int i=0; i<_actions.length; i++) actions.put(getActionHash(plugin, _actions[i]), _actions[i]);
				
				// register paramaters
				Parameter[] _parameters = plugin.getParameters();
				for (int i=0; i<_parameters.length; i++) parameters.put(getParameterHash(plugin, _parameters[i]), _parameters[i]);
				
				// register list paramaters
				ListParameter[] _listParameters = plugin.getListParameters();
				for (int i=0; i<_listParameters.length; i++) listParameters.put(getListParameterHash(plugin, _listParameters[i]), _listParameters[i]);
				
				// register push paramaters
				PushParameter[] _pushParameters = plugin.getPushParameters();
				for (int i=0; i<_pushParameters.length; i++) pushParameters.put(getPushParameterHash(plugin, _pushParameters[i]), _pushParameters[i]);

				// register table paramaters
				TableParameter[] _tableParameters = plugin.getTableParameters();
				for (int i=0; i<_tableParameters.length; i++) tableParameters.put(getTableParameterHash(plugin, _tableParameters[i]), _tableParameters[i]);
				
				AgentWriteLog(LogLevel.DEBUG, "SubAgent.loadPlugin actions=" + actions);
				AgentWriteLog(LogLevel.DEBUG, "SubAgent.loadPlugin parameters=" + parameters);
				AgentWriteLog(LogLevel.DEBUG, "SubAgent.loadPlugin listParameters=" + listParameters);
				AgentWriteLog(LogLevel.DEBUG, "SubAgent.loadPlugin pushParameters=" + pushParameters);
				AgentWriteLog(LogLevel.DEBUG, "SubAgent.loadPlugin tableParameters=" + tableParameters);
				return true;
			}
		}
		return false;
	}

	protected Plugin createPluginWithClassname(String classname, Config config) throws Exception {
		Class pluginClass = Class.forName(classname);
		AgentWriteLog(LogLevel.DEBUG, "SubAgent.createPluginWithClassname loaded class " + pluginClass);
		Plugin plugin = instantiatePlugin(pluginClass, config);
		AgentWriteLog(LogLevel.DEBUG, "SubAgent.createPluginWithClassname created instance " + plugin);
		return plugin;
	}
	
	protected  Plugin createPluginWithJar(String jarFile, Config config) throws Exception {
		URLClassLoader classLoader = new URLClassLoader(new URL[] {new URL(jarFile)});
		URL url = classLoader.findResource(MANIFEST_PATH);
		Manifest manifest = new Manifest(url.openStream());
		Attributes attributes = manifest.getMainAttributes();
		String pluginClassName = attributes.getValue(PLUGIN_CLASSNAME_ATTRIBUTE_NAME);
		if (pluginClassName == null) throw new Exception("Failed to find " + PLUGIN_CLASSNAME_ATTRIBUTE_NAME + " attribute in manifest of " + jarFile);
		Class<?> pluginClass = classLoader.loadClass(pluginClassName);
		Plugin plugin = instantiatePlugin(pluginClass, config);
		return plugin;
	}
	
	// uses reflection to create an instance of Plugin
	protected Plugin instantiatePlugin(Class pluginClass, Config config) throws Exception {
		Plugin plugin = null;
		Constructor<Plugin> constructor = pluginClass.getDeclaredConstructor(org.netxms.agent.Config.class);
		if (constructor != null) {
		      plugin = constructor.newInstance(config);
		} else {
		      throw new Exception("Failed to find constructor Plugin(Config) in class " + pluginClass);
		}
		return plugin;
	}

	// to be called from native subagent
	public String parameterHandler(final String param, final String id) {
		// retrieve appropriate Parameter from cache
		Parameter parameter = getParameter(id);
		if (parameter != null) {
		      return parameter.getValue(param);
		}
		return null;
	}
	
	// to be called from native subagent
	public String[] listParameterHandler(final String param, final String id) {
		// retrieve appropriate ListParameter from cache
		ListParameter listParameter = getListParameter(id);
		if (listParameter != null) {
		      return listParameter.getValues(param);
		}
		return null;
	}
	
	// to be called from native subagent
	public String[][] tableParameterHandler(final String param, final String id) {
		// retrieve appropriate TableParameter from cache
		AgentWriteLog(LogLevel.DEBUG, "SubAgent.tableParameterHandler(param=" + param + ", id=" + id + ")");
		TableParameter tableParameter = getTableParameter(id);
		if (tableParameter != null) {
		      AgentWriteLog(LogLevel.DEBUG, "SubAgent.tableParameterHandler(param=" + param + ", id=" + id + ") returning " + tableParameter.getValues(param));
		      return tableParameter.getValues(param);
		}
		return null;
	}
	
	// to be called from native subagent
	public void actionHandler(final String param, final String[] args, final String id) {
		// retrieve appropriate Action from cache
		Action action = getAction(id);
		if (action != null) {
		      action.execute(param, args);
		}
	}

	public String[] getActionIds() {
		return actions.keySet().toArray(new String[0]);
	}
	
	public String[] getParameterIds() {
		return parameters.keySet().toArray(new String[0]);
	}
	
	public String[] getListParameterIds() {
		return listParameters.keySet().toArray(new String[0]);
	}
	
	public String[] getPushParameterIds() {
		return pushParameters.keySet().toArray(new String[0]);
	}
	
	public String[] getTableParameterIds() {
		return tableParameters.keySet().toArray(new String[0]);
	}

	protected String getActionHash(Plugin plugin, Action action) {
		return plugin.getName() + " / " + action.getName();
	}
	
	protected Action getAction(String id) {
		return actions.get(id);
	}
	
	protected String getParameterHash(Plugin plugin, Parameter parameter) {
		return plugin.getName() + " / " + parameter.getName();
	}
	
	protected Parameter getParameter(String id) {
		return parameters.get(id);
	}
	
	protected String getListParameterHash(Plugin plugin, ListParameter listParameter) {
		return plugin.getName() + " / " + listParameter.getName();
	}
	
	protected ListParameter getListParameter(String id) {
		return listParameters.get(id);
	}
	
	protected String getPushParameterHash(Plugin plugin, PushParameter pushParameter) {
		return plugin.getName() + " / " + pushParameter.getName();
	}
	
	protected PushParameter getPushParameter(String id) {
		return pushParameters.get(id);
	}
	
	protected String getTableParameterHash(Plugin plugin, TableParameter tableParameter) {
		return plugin.getName() + " / " + tableParameter.getName();
	}
	
	protected TableParameter getTableParameter(String id) {
		return tableParameters.get(id);
	}

	private SubAgent(Config config) {
		// will be invoked by native wrapper only

		plugins = new HashMap<String, Plugin>();
		actions = new HashMap<String, Action>();
		parameters = new HashMap<String, Parameter>();
		listParameters = new HashMap<String, ListParameter>();
		pushParameters = new HashMap<String, PushParameter>();
		tableParameters = new HashMap<String, TableParameter>();

		this.config = config;
		AgentWriteLog(LogLevel.DEBUG, "Java SubAgent created");
		
		// load all Plugins
		ConfigEntry configEntry = config.getEntry("/Java/Plugin");
		if (configEntry != null) {
			for (int i=0; i<configEntry.getValueCount(); i++) {
			      String entry = configEntry.getValue(i).trim();
			      loadPlugin(entry, config);
			}
		} else {
			AgentWriteLog(LogLevel.DEBUG, "No Plugin defined in Java section!");
		}
	}
	
}
