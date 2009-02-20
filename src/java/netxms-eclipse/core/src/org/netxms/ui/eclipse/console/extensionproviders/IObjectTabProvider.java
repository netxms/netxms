package org.netxms.ui.eclipse.console.extensionproviders;

import org.eclipse.swt.custom.CTabFolder;

public interface IObjectTabProvider
{
	public static final String EXTENSION_ID = "org.netxms.nxmc.core.objecttab";

	ObjectTab createObjectTab(final CTabFolder tabFolder);
}
