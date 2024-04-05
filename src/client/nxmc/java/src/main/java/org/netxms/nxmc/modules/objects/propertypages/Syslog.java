/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Syslog" property page for node
 */
public class Syslog extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(Syslog.class);

   private AbstractNode node;
   private LabeledText codepage;
   private NXCSession session;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public Syslog(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(Syslog.class).tr("Syslog"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "communication.syslog";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getParentId()
    */
   @Override
   public String getParentId()
   {
      return "communication";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof AbstractNode;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      node = (AbstractNode)object;
      session = Registry.getSession();

      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout dialogLayout = new GridLayout();
      dialogLayout.marginWidth = 0;
      dialogLayout.marginHeight = 0;
      dialogLayout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(dialogLayout);

      codepage = new LabeledText(dialogArea, SWT.NONE);
      codepage.setLabel("Codepage");
      codepage.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      codepage.setText(node.getSyslogCodepage());

      return dialogArea;
   }

   /**
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   protected boolean applyChanges(final boolean isApply)
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(node.getObjectId());
      
      if (isApply)
         setValid(false);

      md.setSyslogCodepage(codepage.getText());

      new Job(i18n.tr("Updating syslog codepage for node {0}", node.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update syslog codepage for node {0}", node.getObjectName());
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> Syslog.this.setValid(true));
         }
      }.start();
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      super.performDefaults();
      codepage.setText("");
   }
}

