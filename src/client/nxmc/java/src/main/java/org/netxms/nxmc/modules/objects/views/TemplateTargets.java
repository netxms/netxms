/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views;

import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Template;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.views.helpers.TemplateTargetsComparator;
import org.netxms.nxmc.modules.objects.views.helpers.TemplateTargetsFilter;
import org.netxms.nxmc.modules.objects.views.helpers.TemplateTargetsLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * List all objects template is applied on
 */
public class TemplateTargets extends ObjectView
{
   private static final I18n i18n = LocalizationHelper.getI18n(TemplateTargets.class);
   
   public static final int COLUMN_ID = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_ZONE = 2;
   public static final int COLUMN_PRIMARY_HOST_NAME = 3;
   public static final int COLUMN_DESCRIPTION = 4;
   
   private SessionListener sessionListener;
   private SortableTableViewer viewer; 
   private TemplateTargetsFilter filter;
   private TemplateTargetsLabelProvider labelProvider;

   /**
    * Constructor
    */
   public TemplateTargets()
   {
      super("Targets", ResourceManager.getImageDescriptor("icons/object-views/nodes.png"), "Targets", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final String[] names = {
            i18n.tr("ID"),
            i18n.tr("Name"),
            i18n.tr("Zone"),
            i18n.tr("Primary host name"),
            i18n.tr("Description"),
            
      };
      final int[] widths = { 60, 300, 300, 300, 300 };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      labelProvider = new TemplateTargetsLabelProvider();
      viewer.setLabelProvider(labelProvider);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new TemplateTargetsComparator(labelProvider));
      filter = new TemplateTargetsFilter(labelProvider);
      setFilterClient(viewer, filter);
      viewer.addFilter(filter);
      WidgetHelper.restoreTableViewerSettings(viewer, "InterfacesView.TableViewer");
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), "InterfacesView.TableViewer");
         }
      });     
      
      createContextMenu();
      
      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.OBJECT_CHANGED)
            {
               AbstractObject object = (AbstractObject)n.getObject();
               if ((object != null) && (getObject() != null)
                     && object.isChildOf(getObject().getObjectId()))
               {
                  viewer.getTable().getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        refresh();
                     }
                  });
               }
            }
         }
      };
      session.addListener(sessionListener);
   }
   
   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new ObjectContextMenuManager(this, viewer, null);

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      if (getObject() != null)
      {
         viewer.setInput(getObject().getChildrenAsArray());
      }
      else
      {
         viewer.setInput(new AbstractNode[0]);
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if ((object != null) && isActive())
         refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context instanceof Template);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(sessionListener);
      super.dispose();
   }

}
