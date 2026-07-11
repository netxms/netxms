/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.TrafficObserver;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

/**
 * "Synchronization" property page for traffic observer objects
 */
public class TrafficObserverSync extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(TrafficObserverSync.class);

   private TrafficObserver observer;
   private Button enableHostAliasSync;
   private LabeledSpinner syncInterval;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public TrafficObserverSync(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(TrafficObserverSync.class).tr("Synchronization"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "trafficObserverSync";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof TrafficObserver);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);
      observer = (TrafficObserver)object;

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      boolean enabled = false;
      int interval = 3600;
      try
      {
         JsonObject hostAliasesConfig = parseSyncConfig(observer.getSyncConfig()).getAsJsonObject("hostAliases");
         if (hostAliasesConfig != null)
         {
            enabled = hostAliasesConfig.has("enabled") && hostAliasesConfig.get("enabled").getAsBoolean();
            if (hostAliasesConfig.has("interval"))
               interval = hostAliasesConfig.get("interval").getAsInt();
         }
      }
      catch(Exception e)
      {
         // malformed sync configuration - fall back to defaults
      }

      enableHostAliasSync = new Button(dialogArea, SWT.CHECK);
      enableHostAliasSync.setText(i18n.tr("Push names of matched nodes to the analyzer as host &aliases"));
      enableHostAliasSync.setSelection(enabled);
      enableHostAliasSync.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      enableHostAliasSync.addListener(SWT.Selection, (e) -> syncInterval.setEnabled(enableHostAliasSync.getSelection()));

      syncInterval = new LabeledSpinner(dialogArea, SWT.NONE);
      syncInterval.setLabel(i18n.tr("Synchronization interval (seconds)"));
      syncInterval.setRange(60, 86400);
      syncInterval.setSelection(interval);
      syncInterval.setEnabled(enabled);
      syncInterval.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false));

      if (!observer.hasCapability(TrafficObserver.CAP_SYNC_HOST_ALIASES))
      {
         Label warning = new Label(dialogArea, SWT.WRAP);
         warning.setText(i18n.tr("Warning: the analyzer backend does not report host alias synchronization capability; synchronization will not run until the backend supports it."));
         warning.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      }

      return dialogArea;
   }

   /**
    * Parse sync configuration JSON, returning empty object on any parse failure.
    *
    * @param syncConfig sync configuration document as string
    * @return parsed configuration or empty object
    */
   private static JsonObject parseSyncConfig(String syncConfig)
   {
      if ((syncConfig != null) && !syncConfig.isEmpty())
      {
         try
         {
            return JsonParser.parseString(syncConfig).getAsJsonObject();
         }
         catch(Exception e)
         {
         }
      }
      return new JsonObject();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      JsonObject config = parseSyncConfig(observer.getSyncConfig());
      JsonObject hostAliases = new JsonObject();
      hostAliases.addProperty("enabled", enableHostAliasSync.getSelection());
      hostAliases.addProperty("interval", syncInterval.getSelection());
      config.add("hostAliases", hostAliases);

      final NXCObjectModificationData md = new NXCObjectModificationData(observer.getObjectId());
      md.setSyncConfig(config.toString());

      if (isApply)
         setValid(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating traffic observer synchronization settings"), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update traffic observer synchronization settings");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> TrafficObserverSync.this.setValid(true));
         }
      }.start();
      return true;
   }
}
