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
package org.netxms.ui.eclipse.objecttools;

import java.lang.reflect.InvocationTargetException;
import java.util.HashMap;
import java.util.Map;
import org.eclipse.core.runtime.IAdapterFactory;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.model.IWorkbenchAdapter;
import org.eclipse.ui.progress.IProgressService;
import org.netxms.client.NXCSession;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Adapter factory for object tools
 */
public class ObjectToolsAdapterFactory implements IAdapterFactory
{
	private final class ToolDetailLoader implements IRunnableWithProgress
	{
		private Display display;
		private final long toolId;
		private ObjectToolDetails result = null;
		private NXCSession session = (NXCSession)ConsoleSharedData.getSession();

		/**
		 * @param toolId
		 * @param display
		 */
		private ToolDetailLoader(long toolId, Display display)
		{
			this.toolId = toolId;
			this.display = display;
		}

		/* (non-Javadoc)
		 * @see org.eclipse.jface.operation.IRunnableWithProgress#run(org.eclipse.core.runtime.IProgressMonitor)
		 */
		@Override
		public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException
		{
			try
			{
				result = session.getObjectToolDetails(toolId);
				display.asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  cache.put(toolId, result);
               }
            });
			}
			catch(final Exception e)
			{
            display.asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  MessageDialogHelper.openError(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), Messages.get().ObjectToolsAdapterFactory_Error, 
                        String.format(Messages.get().ObjectToolsAdapterFactory_LoaderErrorText, e.getLocalizedMessage()));
               }
            });
			}
		}

		public ObjectToolDetails getResult()
		{
			return result;
		}
	}

	@SuppressWarnings("rawtypes")
	private static final Class[] supportedClasses = 
	{
		IWorkbenchAdapter.class,
		ObjectToolDetails.class
	};
	
	private static Map<Long, ObjectToolDetails> cache = new HashMap<Long, ObjectToolDetails>();

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapter(java.lang.Object, java.lang.Class)
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Object getAdapter(Object adaptableObject, Class adapterType)
	{
		if (adaptableObject.getClass() != ObjectTool.class)
			return null;

		// Adapt to tool details
		if (adapterType == ObjectToolDetails.class)
		{
			final long toolId = ((ObjectTool)adaptableObject).getId();
			ObjectToolDetails details = cache.get(toolId);
			if (details == null)
			{
				ToolDetailLoader job = new ToolDetailLoader(toolId, PlatformUI.getWorkbench().getDisplay());
				IProgressService service = PlatformUI.getWorkbench().getProgressService();
				try
				{
					service.busyCursorWhile(job);
					details = job.getResult();
				}
				catch(Exception e)
				{
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			return details;
		}
		
		// Adapt to IWorkbenchAdapter
		if (adapterType == IWorkbenchAdapter.class)
		{
			return new IWorkbenchAdapter() {
				@Override
				public Object getParent(Object o)
				{
					return null;
				}
				
				@Override
				public String getLabel(Object o)
				{
					return ((ObjectTool)o).getDisplayName();
				}
				
				@Override
				public ImageDescriptor getImageDescriptor(Object object)
				{
					return null;
				}
				
				@Override
				public Object[] getChildren(Object o)
				{
					return null;
				}
			};
		}
		
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapterList()
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Class[] getAdapterList()
	{
		return supportedClasses;
	}
	
	/**
	 * Get details object from c ache
	 * 
	 * @param toolId tool ID
	 * @return
	 */
	public static ObjectToolDetails getDetailsFromCache(long toolId)
	{
		return cache.get(toolId);
	}
	
	/**
	 * Remove tool details from cache
	 * 
	 * @param toolId tool ID
	 */
	public static void deleteFromCache(long toolId)
	{
		cache.remove(toolId);
	}
	
	/**
	 * Clear cache
	 */
	public static void clearCache()
	{
		cache.clear();
	}
}
