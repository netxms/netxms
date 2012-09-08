/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.filemanager;

import org.eclipse.core.runtime.IAdapterFactory;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.model.IWorkbenchAdapter;
import org.netxms.client.ServerFile;

/**
 * Adapter factory for ServerFile objects
 *
 */
public class ServerFileAdapterFactory implements IAdapterFactory
{
	@SuppressWarnings("rawtypes")
	private static final Class[] supportedClasses = 
	{
		IWorkbenchAdapter.class
	};

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapterList()
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Class[] getAdapterList()
	{
		return supportedClasses;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapter(java.lang.Object, java.lang.Class)
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Object getAdapter(Object adaptableObject, Class adapterType)
	{
		if ((adapterType == IWorkbenchAdapter.class) && (adaptableObject instanceof ServerFile))
		{
			return new IWorkbenchAdapter() {
				@Override
				public Object[] getChildren(Object o)
				{
					return null;
				}

				@Override
				public ImageDescriptor getImageDescriptor(Object object)
				{
					String[] parts = ((ServerFile)object).getName().split("."); //$NON-NLS-1$
					if (parts.length < 2)
						return Activator.getImageDescriptor("icons/file.png"); //$NON-NLS-1$

					String ext = parts[parts.length - 1];
					
					if (ext.equalsIgnoreCase("exe")) //$NON-NLS-1$
						return Activator.getImageDescriptor("icons/exec.png"); //$NON-NLS-1$
					
					if (ext.equalsIgnoreCase("pdf")) //$NON-NLS-1$
						return Activator.getImageDescriptor("icons/pdf.png"); //$NON-NLS-1$
					
					if (ext.equalsIgnoreCase("avi") || ext.equalsIgnoreCase("mkv") || ext.equalsIgnoreCase("mov") || ext.equalsIgnoreCase("wma")) //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$
						return Activator.getImageDescriptor("icons/video.png"); //$NON-NLS-1$
					
					if (ext.equalsIgnoreCase("tar") || ext.equalsIgnoreCase("gz") ||  //$NON-NLS-1$ //$NON-NLS-2$
					    ext.equalsIgnoreCase("tgz") || ext.equalsIgnoreCase("zip") || //$NON-NLS-1$ //$NON-NLS-2$
					    ext.equalsIgnoreCase("rar") || ext.equalsIgnoreCase("7z") || //$NON-NLS-1$ //$NON-NLS-2$
					    ext.equalsIgnoreCase("bz2") || ext.equalsIgnoreCase("lzma")) //$NON-NLS-1$ //$NON-NLS-2$
						return Activator.getImageDescriptor("icons/archive.png"); //$NON-NLS-1$
					
					return Activator.getImageDescriptor("icons/file.png"); //$NON-NLS-1$
				}

				@Override
				public String getLabel(Object o)
				{
					return ((ServerFile)o).getName();
				}

				@Override
				public Object getParent(Object o)
				{
					return null;
				}
			};
		}
		return null;
	}
}
