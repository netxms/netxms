package org.netxms.ui.eclipse.imagelibrary;

import java.io.IOException;
import org.eclipse.ui.IStartup;
import org.netxms.client.NXCException;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;

public class Startup implements IStartup
{

	@Override
	public void earlyStartup()
	{
		try
		{
			ImageProvider.getInstance().syncMetaData();
		}
		catch(NXCException e)
		{
			// FIXME
			e.printStackTrace();
		}
		catch(IOException e)
		{
			// FIXME
			e.printStackTrace();
		}
	}

}
