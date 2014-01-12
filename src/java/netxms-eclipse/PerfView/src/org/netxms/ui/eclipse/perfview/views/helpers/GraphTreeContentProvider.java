/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.perfview.views.helpers;

import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.perfview.Messages;

/**
 * Content provider for predefined graph tree
 *
 */
public class GraphTreeContentProvider implements ITreeContentProvider
{
	private GraphFolder rootFolder;
	private Map<GraphSettings, GraphFolder> parentFolders = new HashMap<GraphSettings, GraphFolder>();
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
	 */
	@Override
	public Object[] getChildren(Object parentElement)
	{
		if (parentElement instanceof GraphFolder)
			return ((GraphFolder)parentElement).getChildObjects();
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
	 */
	@Override
	public Object getParent(Object element)
	{
		if (element instanceof GraphFolder)
			return ((GraphFolder)element).getParent();
		return parentFolders.get(element);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
	 */
	@Override
	public boolean hasChildren(Object element)
	{
		if (element instanceof GraphFolder)
			return ((GraphFolder)element).hasChildren();
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
	 */
	@Override
	public Object[] getElements(Object inputElement)
	{
		return new Object[] { rootFolder };
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
	 */
	@Override
	public void dispose()
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		parentFolders.clear();
		rootFolder = new GraphFolder(Messages.GraphTreeContentProvider_Root, null);
		
		List<GraphSettings> gs = (List<GraphSettings>)newInput;
		if (gs != null)
		{
			Collections.sort(gs, new Comparator<GraphSettings>() {
				@Override
				public int compare(GraphSettings arg0, GraphSettings arg1)
				{
					return arg0.getName().replace("&", "").compareToIgnoreCase(arg1.getName().replace("&", "")); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$
				}
			});
		
			Map<String, GraphFolder> folders = new HashMap<String, GraphFolder>();
			for(int i = 0; i < gs.size(); i++)
			{
				String[] path = gs.get(i).getName().split("\\-\\>"); //$NON-NLS-1$
			
				GraphFolder root = rootFolder;
				for(int j = 0; j < path.length - 1; j++)
				{
					String key = path[j].replace("&", ""); //$NON-NLS-1$ //$NON-NLS-2$
					GraphFolder curr = folders.get(key);
					if (curr == null)
					{
						curr = new GraphFolder(path[j], root);
						folders.put(key, curr);
						root.addFolder(curr);
					}
					root = curr;
				}
	
				root.addGraph(gs.get(i));
				parentFolders.put(gs.get(i), root);
			}
		}
	}
}
