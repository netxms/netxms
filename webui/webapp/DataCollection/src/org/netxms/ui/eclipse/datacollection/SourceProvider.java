/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.AbstractSourceProvider;
import org.eclipse.ui.ISources;
import org.eclipse.ui.services.IServiceLocator;
import org.netxms.ui.eclipse.datacollection.api.SummaryTablesCache;

/**
 * Data collection source provider
 */
public class SourceProvider extends AbstractSourceProvider
{
   private static final String INSTANCE_ATTRIBUTE = "netxms.datacollection.SourceProvider"; //$NON-NLS-1$
   private static final String SUMMARY_TABLES_EXIST = "org.netxms.ui.eclipse.datacollection.SummaryTablesExist";
   
   private static final String[] PROVIDED_SOURCE_NAMES = 
      { 
         SUMMARY_TABLES_EXIST 
      };
   
   private Display display;
   private Map<String, Object> stateMap = new HashMap<String, Object>(1);
   
   /**
    * Get source provider instance.
    * 
    * @return
    */
   public static SourceProvider getInstance(Display display)
   {
      return (SourceProvider)RWT.getUISession(display).getAttribute(INSTANCE_ATTRIBUTE);
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.AbstractSourceProvider#initialize(org.eclipse.ui.services.IServiceLocator)
    */
   @Override
   public void initialize(IServiceLocator locator)
   {
      super.initialize(locator);
      stateMap.put(SUMMARY_TABLES_EXIST, false);
      display = Display.getCurrent();
      RWT.getUISession(display).setAttribute(INSTANCE_ATTRIBUTE, this);
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
    * @see org.eclipse.ui.ISourceProvider#getCurrentState()
    */
   @Override
   public Map<?, ?> getCurrentState()
   {
      return stateMap;
   }
   
   /**
    * Update provider
    */
   public void update()
   {
      display.asyncExec(new Runnable() {
         @Override
         public void run()
         {
            stateMap.put(SUMMARY_TABLES_EXIST, !SummaryTablesCache.getInstance().isEmpty());
            fireSourceChanged(ISources.WORKBENCH, getCurrentState());
         }
      });
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISourceProvider#dispose()
    */
   @Override
   public void dispose()
   {
      RWT.getUISession().removeAttribute(INSTANCE_ATTRIBUTE);
   }
}
