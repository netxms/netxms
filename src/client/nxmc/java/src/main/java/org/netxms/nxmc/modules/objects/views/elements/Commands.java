/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views.elements;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.CommandBox;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objecttools.ObjectToolExecutor;
import org.netxms.nxmc.modules.objecttools.ObjectToolsCache;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * "Commands" element
 */
public class Commands extends OverviewPageElement
{
   private final I18n i18n = LocalizationHelper.getI18n(Commands.class);

	private CommandBox commandBox;
	private Action actionWakeup;
	
	/**
    * @param parent
    * @param anchor
    * @param objectView
    */
   public Commands(Composite parent, OverviewPageElement anchor, ObjectView objectView)
	{
      super(parent, anchor, objectView);
		createActions();
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
      final NXCSession session = Registry.getSession();
		
      actionWakeup = new Action(i18n.tr("Wakeup node using Wake-on-LAN")) {
			@Override
			public void run()
			{
				final AbstractObject object = getObject();
            new Job(i18n.tr("Wakeup node"), getObjectView()) {
					@Override
               protected void run(IProgressMonitor monitor) throws Exception
					{
						session.wakeupNode(object.getObjectId());
					}

					@Override
					protected String getErrorMessage()
					{
                  return i18n.tr("Cannot send wake-on-LAN packet to node");
					}
				}.start();
			}
		};
      actionWakeup.setImageDescriptor(ResourceManager.getImageDescriptor("icons/wake-on-lan.png"));
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
	@Override
	protected String getTitle()
	{
      return i18n.tr("Commands");
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#onObjectChange()
    */
	@Override
	protected void onObjectChange()
	{
		commandBox.deleteAll(false);
		if (getObject() instanceof AbstractNode)
		{
		   ObjectTool[] tools = ObjectToolsCache.getInstance().getTools();
		   for(final ObjectTool tool : tools)
		   {
            if (!tool.isVisibleInCommands() || !tool.isEnabled() || !tool.isApplicableForObject(getObject()))
		         continue;

            final Set<ObjectContext> nodes = new HashSet<ObjectContext>(1);
            nodes.add(new ObjectContext((AbstractNode)getObject(), null, getObject().getObjectId()));
            if (!ObjectToolExecutor.isToolAllowed(tool, nodes))
               continue;
		      
		      final Action action = new Action(tool.getCommandDisplayName()) {
               @Override
               public void run()
               {
                  ObjectToolExecutor.execute(nodes, nodes, tool, new ViewPlacement(getObjectView()));
               }
            };
            ImageDescriptor icon = ObjectToolsCache.getInstance().findIcon(tool.getId());
            if (icon != null)
               action.setImageDescriptor(icon);
	         commandBox.add(action, false);
		   }
		}
		else if (getObject() instanceof Interface)
		{
			commandBox.add(actionWakeup, false);
		}
		commandBox.rebuild();
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#createClientArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createClientArea(Composite parent)
	{
		commandBox = new CommandBox(parent, SWT.NONE);
		return commandBox;
	} 

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
	@Override
	public boolean isApplicableForObject(AbstractObject object)
	{
		return (object instanceof Node) || (object instanceof Interface);
	}
}
