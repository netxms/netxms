package org.netxms.nxmc.core.extensionproviders;

import org.eclipse.swt.custom.CTabFolder;

public interface IObjectTabProvider
{
	public static final String EXTENSION_ID = "org.netxms.nxmc.core.objecttab";

	ObjectTab createObjectTab(final CTabFolder tabFolder);
}
