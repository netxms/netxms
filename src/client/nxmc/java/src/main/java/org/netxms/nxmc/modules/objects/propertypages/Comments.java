/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Comments" property page for NetXMS object
 */
public class Comments extends ObjectPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(Comments.class);

   private Text comments;
   private Button checkMarkdown;
   private String initialComments;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public Comments(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(Comments.class).tr("Comments"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "comments";
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      initialComments = object.getCommentsSource();
      if ((initialComments == null) || initialComments.isEmpty())
         initialComments = AbstractObject.MARKDOWN_COMMENTS_INDICATOR;

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      comments = new Text(dialogArea, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.widthHint = 0;
      gd.heightHint = 0;
      comments.setLayoutData(gd);

      checkMarkdown = new Button(dialogArea, SWT.CHECK);
      checkMarkdown.setText(i18n.tr("Use &markdown"));

      if (initialComments.startsWith(AbstractObject.MARKDOWN_COMMENTS_INDICATOR))
      {
         checkMarkdown.setSelection(true);
         comments.setText(initialComments.substring(AbstractObject.MARKDOWN_COMMENTS_INDICATOR.length()));
      }
      else
      {
         comments.setText(initialComments);
      }

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      final String updatedComments = checkMarkdown.getSelection() ? AbstractObject.MARKDOWN_COMMENTS_INDICATOR + comments.getText() : comments.getText();

      if (initialComments.equals(updatedComments))
         return true; // Nothing to apply

      if (isApply)
         setValid(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating comments for object {0}", object.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateObjectComments(object.getObjectId(), updatedComments);
            initialComments = updatedComments;
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot change comments");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> Comments.this.setValid(true));
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
      comments.setText("");
      checkMarkdown.setSelection(true);
   }
}
