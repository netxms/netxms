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
package org.netxms.ui.eclipse.objectview.services;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.ui.AbstractSourceProvider;
import org.eclipse.ui.ISources;
import org.eclipse.ui.services.IServiceLocator;
import org.netxms.ui.eclipse.objectview.Activator;

/**
 * Source provider
 */
public class SourceProvider extends AbstractSourceProvider
{
	public static final String ACTIVE_TAB = "nxmcObjectViewActiveTab";
	
	private static final String[] PROVIDED_SOURCE_NAMES = new String[] { ACTIVE_TAB };
	private static final Map<String, Object> stateMap = new HashMap<String, Object>(1);

	/* (non-Javadoc)
	 * @see org.eclipse.ui.AbstractSourceProvider#initialize(org.eclipse.ui.services.IServiceLocator)
	 */
	@Override
	public void initialize(IServiceLocator locator)
	{
		super.initialize(locator);
		Activator.getDefault().setSourceProvider(this);
		stateMap.put(ACTIVE_TAB, null);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISourceProvider#getCurrentState()
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Map getCurrentState()
	{
		return stateMap;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISourceProvider#getProvidedSourceNames()
	 */
	@Override
	public String[] getProvidedSourceNames()
	{
		return PROVIDED_SOURCE_NAMES;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISourceProvider#dispose()
	 */
	@Override
	public void dispose()
	{
	}
	
	/**
	 * @param name
	 * @param value
	 */
	public void updateProperty(String name, Object value)
	{
		stateMap.put(name, value);
		fireSourceChanged(ISources.WORKBENCH, getCurrentState());
	}
}
