package org.netxms.ui.eclipse.scripting;

import java.io.File;
import java.util.Properties;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.osgi.framework.BundleContext;
import org.python.core.PySystemState;

/**
 * The activator class controls the plug-in life cycle
 */
public class Activator extends AbstractUIPlugin
{
	// The plug-in ID
	public static final String PLUGIN_ID = "org.netxms.ui.eclipse.scripting"; //$NON-NLS-1$

	// The shared instance
	private static Activator plugin;

	/**
	 * The constructor
	 */
	public Activator()
	{
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.plugin.AbstractUIPlugin#start(org.osgi.framework.BundleContext)
	 */
	public void start(BundleContext context) throws Exception
	{
		super.start(context);
		plugin = this;
		initJython();
	}

	/**
	 * Initialize Jython
	 */
	private void initJython()
	{
		final Properties postProperties = new Properties();
		final String tempDirectory = System.getProperty("java.io.tmpdir") + File.separator + "nxshell";
		postProperties.setProperty("python.cachedir", tempDirectory);
		postProperties.setProperty("python.cachedir.skip", "false");
		PySystemState.initialize(PySystemState.getBaseProperties(), postProperties, new String[0]);
	}
	
	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.plugin.AbstractUIPlugin#stop(org.osgi.framework.BundleContext)
	 */
	public void stop(BundleContext context) throws Exception
	{
		plugin = null;
		super.stop(context);
	}

	/**
	 * Returns the shared instance
	 * 
	 * @return the shared instance
	 */
	public static Activator getDefault()
	{
		return plugin;
	}

	/**
	 * Returns an image descriptor for the image file at the given plug-in relative path
	 * 
	 * @param path the path
	 * @return the image descriptor
	 */
	public static ImageDescriptor getImageDescriptor(String path)
	{
		return imageDescriptorFromPlugin(PLUGIN_ID, path);
	}
}
