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
package org.netxms.nxmc.modules.objects.actions;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectPublicAccessDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Enable public access to object (network map or dashboard) via anonymous user
 */
public class EnableObjectPublicAccess extends ObjectAction<AbstractObject>
{
   private final I18n i18n = LocalizationHelper.getI18n(EnableObjectPublicAccess.class);

   /**
    * Create action
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public EnableObjectPublicAccess(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(AbstractObject.class, LocalizationHelper.getI18n(EnableObjectPublicAccess.class).tr("Enable public access..."), viewPlacement, selectionProvider);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<AbstractObject> objects)
   {      
      final AbstractObject object = objects.get(0);
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Setting up public access"), null, getMessageArea()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final String token = session.enableAnonymousObjectAccess(object.getObjectId());
            runInUIThread(() -> {
               StringBuilder url = new StringBuilder(session.getClientConfigurationHint("BaseURL", "https://{server-name}").replace("{server-name}", session.getServerAddress()));
               if (url.charAt(url.length() - 1) != '/')
                  url.append('/');
               url.append("nxmc-light.app?auto&kiosk-mode=true&token=");
               url.append(token);
               url.append((object instanceof Dashboard) ? "&dashboard=" : "&map=");
               url.append(object.getObjectId());
               new ObjectPublicAccessDialog(getShell(), token, url.toString()).open();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot setup public access for object \"%s\""), object.getObjectName());
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#isValidForSelection(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      return (selection.size() == 1) && ((selection.getFirstElement() instanceof NetworkMap) || (selection.getFirstElement() instanceof Dashboard));
   }
}
