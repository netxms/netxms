package org.netxms.ui.eclipse.charts;

import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.eclipse.ui.progress.IProgressService;
import org.netxms.ui.eclipse.charts.views.HistoryGraph;
import org.osgi.framework.BundleContext;
import org.swtchart.LineStyle;

/**
 * The activator class controls the plug-in life cycle
 */
public class Activator extends AbstractUIPlugin
{
	// The plug-in ID
	public static final String PLUGIN_ID = "org.netxms.ui.eclipse.charts";

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
	 * @see
	 * org.eclipse.ui.plugin.AbstractUIPlugin#start(org.osgi.framework.BundleContext
	 * )
	 */
	public void start(BundleContext context) throws Exception
	{
		super.start(context);
		plugin = this;

		// Register icon for our jobs
		IProgressService service = PlatformUI.getWorkbench().getProgressService();
	   service.registerIconForFamily(getImageDescriptor("icons/graph.png"), HistoryGraph.JOB_FAMILY);
	   
	   // Set default values for preferences
	   IPreferenceStore ps = getPreferenceStore();
	   
	   ps.setDefault("Chart.Colors.Background", "240,240,240");
	   ps.setDefault("Chart.Colors.Data.0", "0,192,0");
	   ps.setDefault("Chart.Colors.Data.1", "165,42,0");
	   ps.setDefault("Chart.Colors.Data.2", "0,64,64");
	   ps.setDefault("Chart.Colors.Data.3", "0,0,255");
	   ps.setDefault("Chart.Colors.Data.4", "255,0,255");
	   ps.setDefault("Chart.Colors.Data.5", "255,0,0");
	   ps.setDefault("Chart.Colors.Data.6", "0,128,128");
	   ps.setDefault("Chart.Colors.Data.7", "0,128,0");
	   ps.setDefault("Chart.Colors.Data.8", "128,128,255");
	   ps.setDefault("Chart.Colors.Data.9", "255,128,0");
	   ps.setDefault("Chart.Colors.Data.10", "128,128,0");
	   ps.setDefault("Chart.Colors.Data.11", "128,0,255");
	   ps.setDefault("Chart.Colors.Data.12", "0,0,0");
	   ps.setDefault("Chart.Colors.Data.13", "0,128,64");
	   ps.setDefault("Chart.Colors.Data.14", "0,128,255");
	   ps.setDefault("Chart.Colors.Data.15", "192,192,192");
	   ps.setDefault("Chart.Colors.PlotArea", "255,255,255");
	   ps.setDefault("Chart.Colors.Selection", "0,0,128");
	   ps.setDefault("Chart.Colors.Title", "0,0,0");
	   ps.setDefault("Chart.Axis.X.Color", "22,22,22");
	   ps.setDefault("Chart.Axis.Y.Color", "22,22,22");
	   ps.setDefault("Chart.Grid.X.Color", "232,232,232");
	   ps.setDefault("Chart.Grid.X.Style", LineStyle.DOT.label);
	   ps.setDefault("Chart.Grid.Y.Color", "232,232,232");
	   ps.setDefault("Chart.Grid.Y.Style", LineStyle.DOT.label);
	   ps.setDefault("Chart.EnableZoom", true);
	   ps.setDefault("Chart.ShowTitle", false);
	   ps.setDefault("Chart.ShowToolTips", true);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.ui.plugin.AbstractUIPlugin#stop(org.osgi.framework.BundleContext
	 * )
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
	 * Returns an image descriptor for the image file at the given plug-in
	 * relative path
	 * 
	 * @param path
	 *           the path
	 * @return the image descriptor
	 */
	public static ImageDescriptor getImageDescriptor(String path)
	{
		return imageDescriptorFromPlugin(PLUGIN_ID, path);
	}
}
