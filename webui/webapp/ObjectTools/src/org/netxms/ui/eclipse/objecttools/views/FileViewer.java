/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objecttools.views;

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.util.Arrays;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * File viewer
 */
public class FileViewer extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.objecttools.views.FileViewer"; //$NON-NLS-1$
	
	private long nodeId;
	private String remoteFileName;
	private File currentFile;
	private Text textViewer;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		// Secondary ID must by in form nodeId&remoteFileName
		String[] parts = site.getSecondaryId().split("&"); //$NON-NLS-1$
		if (parts.length != 2)
			throw new PartInitException("Internal error"); //$NON-NLS-1$
		
		nodeId = Long.parseLong(parts[0]);
		AbstractObject object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(nodeId);
		if ((object == null) || (object.getObjectClass() != AbstractObject.OBJECT_NODE))
			throw new PartInitException(Messages.get().FileViewer_InvalidObjectID);
		
		try
		{
			remoteFileName = URLDecoder.decode(parts[1], "UTF-8"); //$NON-NLS-1$
		}
		catch(UnsupportedEncodingException e)
		{
			throw new PartInitException("Internal error", e); //$NON-NLS-1$
		}
		
		setPartName(object.getObjectName() + ": " + remoteFileName); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		textViewer = new Text(parent, SWT.H_SCROLL | SWT.V_SCROLL);
		textViewer.setEditable(false);
		textViewer.setFont(JFaceResources.getTextFont());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		textViewer.setFocus();
	}

	/**
	 * @param file
	 */
	public void showFile(File file)
	{
		currentFile = file;
		textViewer.setText(loadFile(currentFile));
	}
	
	/**
	 * @param file
	 */
	private String loadFile(File file)
	{
		StringBuilder content = new StringBuilder();
		FileReader reader = null;
		char[] buffer = new char[32768];
		try
		{
			reader = new FileReader(file);
			int size = 0;
			while(size < 8192000)
			{
				int count = reader.read(buffer);
				if (count == -1)
					break;
				if (count == buffer.length)
				{
					content.append(buffer);
				}
				else
				{
					content.append(Arrays.copyOf(buffer, count));
				}
				size += count;
			}
		}
		catch(IOException e)
		{
			e.printStackTrace();
		}
		finally
		{
			if (reader != null)
			{
				try
				{
					reader.close();
				}
				catch(IOException e)
				{
				}
			}
		}
		return content.toString();
	}
}
