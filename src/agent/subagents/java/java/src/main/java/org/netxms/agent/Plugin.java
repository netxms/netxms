package org.netxms.agent;

public abstract class Plugin {
	// constructor used by PluginManager
	public Plugin(Config config) {
	}

	public abstract String getName();
	public abstract String getVersion();

	public abstract void init(Config config);
	public abstract void shutdown();

	public abstract Parameter[] getParameters();
	public abstract PushParameter[] getPushParameters();
	public abstract ListParameter[] getListParameters();
	public abstract TableParameter[] getTableParameters();
	public abstract Action[] getActions();

	// TODO processCommands
}
