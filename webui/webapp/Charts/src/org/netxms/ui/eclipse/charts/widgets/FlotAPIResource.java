package org.netxms.ui.eclipse.charts.widgets;

import org.eclipse.rwt.resources.IResource;
import org.eclipse.rwt.resources.IResourceManager.RegisterOptions;

public class FlotAPIResource implements IResource {

	@Override
	public ClassLoader getLoader() {
		return this.getClass().getClassLoader();
	}

	@Override
	public String getLocation() {
		return "org/netxms/ui/eclipse/charts/widgets/flot.js";
	}

	@Override
	public String getCharset() {
		return "UTF-8";
	}

	@Override
	public RegisterOptions getOptions() {
		return RegisterOptions.VERSION_AND_COMPRESS;
	}

	@Override
	public boolean isJSLibrary() {
		return true;
	}

	@Override
	public boolean isExternal() {
		return false;
	}

}
