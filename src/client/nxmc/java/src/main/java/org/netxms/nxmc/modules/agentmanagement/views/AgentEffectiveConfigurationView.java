/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.agentmanagement.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.agent.config.AgentConfigurationFile;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Read-only view of agent's effective configuration (master configuration file merged with additional
 * configuration files and policies). Shows cached copy if agent is not reachable.
 */
public class AgentEffectiveConfigurationView extends AdHocObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(AgentEffectiveConfigurationView.class);

   private Text textArea;

   /**
    * Create effective configuration view for given node.
    *
    * @param node node object
    * @param contextId context object ID
    */
   public AgentEffectiveConfigurationView(Node node, long contextId)
   {
      super(LocalizationHelper.getI18n(AgentEffectiveConfigurationView.class).tr("Effective Agent Configuration"), ResourceManager.getImageDescriptor("icons/object-views/agent-config.png"),
            "AgentEffectiveConfigurationView", node.getObjectId(), contextId, false);
   }

   /**
    * Create view for cloning.
    */
   protected AgentEffectiveConfigurationView()
   {
      super(LocalizationHelper.getI18n(AgentEffectiveConfigurationView.class).tr("Effective Agent Configuration"), ResourceManager.getImageDescriptor("icons/object-views/agent-config.png"),
            "AgentEffectiveConfigurationView", 0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View origin)
   {
      super.postClone(origin);
      textArea.setText(((AgentEffectiveConfigurationView)origin).textArea.getText());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      textArea = new Text(parent, SWT.MULTI | SWT.READ_ONLY | SWT.H_SCROLL | SWT.V_SCROLL);
      textArea.setFont(JFaceResources.getTextFont());
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      textArea.setFocus();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      clearMessages();
      new Job(i18n.tr("Loading effective agent configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final AgentConfigurationFile config = session.readAgentEffectiveConfiguration(getObjectId());
            runInUIThread(() -> {
               textArea.setText(config.getContent());
               if (config.isCached())
               {
                  addMessage(MessageArea.WARNING, i18n.tr("Agent is not reachable. Showing cached copy of configuration from {0}.",
                        DateFormatFactory.getDateTimeFormat().format(config.getCacheTime())), true);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load effective agent configuration");
         }
      }.start();
   }
}
