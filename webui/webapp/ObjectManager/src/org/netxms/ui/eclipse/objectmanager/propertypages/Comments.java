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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Comments" property page for NetXMS object
 *
 */
public class Comments extends PropertyPage
{
   private AbstractObject object;
   private Text comments;
   private Button checkMarkdown;
   private String initialComments;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      object = (AbstractObject)getElement().getAdapter(AbstractObject.class);
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
      checkMarkdown.setText("Use &markdown");

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
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   protected void applyChanges(final boolean isApply)
   {
      final String updatedComments = checkMarkdown.getSelection() ? AbstractObject.MARKDOWN_COMMENTS_INDICATOR + comments.getText() : comments.getText();

      if (initialComments.equals(updatedComments))
         return; // Nothing to apply

      if (isApply)
         setValid(false);

		final NXCSession session = ConsoleSharedData.getSession();
		new ConsoleJob(String.format(Messages.get().Comments_JobName, object.getObjectName()), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.updateObjectComments(object.getObjectId(), updatedComments);
				initialComments = updatedComments;
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().Comments_JobError;
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> Comments.this.setValid(true));
			}
		}.start();
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      super.performDefaults();
      comments.setText(""); //$NON-NLS-1$
      checkMarkdown.setSelection(true);
   }
}
