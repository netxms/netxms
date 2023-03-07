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
import org.netxms.nxmc.base.actions.ObjectAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
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
    * @param text action's text
    * @param view owning view
    * @param selectionProvider associated selection provider
    */
   public ForcedPolicyDeploymentAction(String text, Perspective perspective, ISelectionProvider selectionProvider)
   {
      super(Template.class, text, perspective, selectionProvider);
   }

   /**
    * @see org.netxms.nxmc.base.actions.ObjectAction#runInternal(java.util.List)
    */
   @Override
   protected void runInternal(List<Template> objects)
   {		
		final NXCSession session = Registry.getSession();		
		for(final Template template : objects)
		{
			new Job(String.format(i18n.tr("Force policy installation"), template.getObjectName()), null, perspective.getMessageArea()) {
				@Override
				protected void run(IProgressMonitor monitor) throws Exception
				{
				   session.forcePolicyInstallation(template.getObjectId());
				   runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     perspective.getMessageArea().addMessage(MessageArea.INFORMATION, String.format(i18n.tr("Policies from template \"%s\" successfully installed"), template.getObjectName()));
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
