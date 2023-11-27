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
package org.netxms.nxmc.modules.objects.actions;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Template;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Action: deploy policy on agent
 */
public class ForcedPolicyDeploymentAction extends ObjectAction<Template>
{
   private final I18n i18n = LocalizationHelper.getI18n(ForcedPolicyDeploymentAction.class);

   /**
    * Create action for forced policy deployment.
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public ForcedPolicyDeploymentAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(Template.class, LocalizationHelper.getI18n(ForcedPolicyDeploymentAction.class).tr("&Force deployment of agent policies"), viewPlacement, selectionProvider);
      setImageDescriptor(ResourceManager.getImageDescriptor("icons/push.png"));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<Template> objects)
   {		
		final NXCSession session = Registry.getSession();		
		for(final Template template : objects)
		{
         new Job(String.format(i18n.tr("Running forced agent policy deployment for template \"%s\""), template.getObjectName()), null, getMessageArea()) {
				@Override
				protected void run(IProgressMonitor monitor) throws Exception
				{
				   session.forcePolicyInstallation(template.getObjectId());
				   runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     getMessageArea().addMessage(MessageArea.INFORMATION, String.format(i18n.tr("Policies from template \"%s\" successfully installed"), template.getObjectName()));
                  }
               });				   
				}

				@Override
				protected String getErrorMessage()
				{
               return String.format(i18n.tr("Unable to force install policies from template \"%s\""), template.getObjectName());
				}
			}.start();
		}
	}
}
