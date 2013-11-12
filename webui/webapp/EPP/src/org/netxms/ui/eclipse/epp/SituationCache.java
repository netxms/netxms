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
package org.netxms.ui.eclipse.epp;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCException;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.situations.Situation;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Local cache for situation objects
 */
public class SituationCache
{
	private static Map<Long, Situation> situations = new HashMap<Long, Situation>();
	
	/**
	 * Initialize cache - expected to be run from non-UI thread
	 */
	public static void init(final NXCSession session, final Display display)
	{
		try
		{
			List<Situation> list = session.getSituations();
			synchronized(situations)
			{
				for(Situation s : list)
					situations.put(s.getId(), s);
			}
			session.subscribe(NXCSession.CHANNEL_SITUATIONS);
		}
		catch(final Exception e)
		{
			// It's a normal situation if user don't have access to situation objects
			// Don't show error message in this case
			if ((e instanceof NXCException) && (((NXCException)e).getErrorCode() == RCC.ACCESS_DENIED))
				return;
			
			new UIJob(display, Messages.get().SituationCache_JobTitle) {
				@Override
				public IStatus runInUIThread(IProgressMonitor monitor)
				{
					MessageDialogHelper.openError(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), Messages.get().SituationCache_Error, Messages.get().SituationCache_ErrorText + e.getLocalizedMessage());
					return Status.OK_STATUS;
				}
			}.schedule();
		}

		session.addListener(new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				switch(n.getCode())
				{
					case NXCNotification.SITUATION_CREATED:
					case NXCNotification.SITUATION_UPDATED:
						synchronized(situations)
						{
							situations.put(n.getSubCode(), (Situation)n.getObject());
						}
						break;
					case NXCNotification.SITUATION_DELETED:
						synchronized(situations)
						{
							situations.remove(n.getSubCode());
						}
						break;
				}
			}
		});
	}
	
	/**
	 * Get list of all situation objects. Array returned is a copy of
	 * cached list and it's modifications will not affect situation cache.
	 * 
	 * @return list of all situation objects
	 */
	public static Situation[] getAllSituations()
	{
		Situation[] list;
		synchronized(situations)
		{
			list = situations.values().toArray(new Situation[situations.size()]);
		}		
		return list;
	}
	
	/**
	 * Find situation object in cache by ID
	 * 
	 * @param id situation object id
	 * @return situation object or null 
	 */
	public static Situation findSituation(long id)
	{
		Situation s;
		synchronized(situations)
		{
			s = situations.get(id);
		}
		return s;
	}
}
