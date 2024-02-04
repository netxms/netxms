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
package org.netxms.nxmc.modules.alarms.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.events.AlarmComment;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ImageCache;
import org.xnap.commons.i18n.I18n;

/**
 * Editor for alarm comments
 */
public class AlarmCommentsEditor extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(AlarmCommentsEditor.class);
   
   private NXCSession session = Registry.getSession();
	private Text text;
	
	/**
	 * @param parent
	 * @param style
	 * @param comment
	 */
   public AlarmCommentsEditor(Composite parent, ImageCache imageCache, AlarmComment comment, HyperlinkAdapter editAction, HyperlinkAdapter deleteAction, boolean enableEdit)
	{
		super(parent, SWT.BORDER);
		
      setBackground(parent.getBackground());

		GridLayout layout = new GridLayout();
		layout.numColumns = 3;
		setLayout(layout);

      final CLabel user = new CLabel(this, SWT.NONE);
		final AbstractUserObject userObject = session.findUserDBObjectById(comment.getUserId(), new Runnable() {         
         @Override
         public void run()
         {
            parent.getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  final AbstractUserObject userObject = session.findUserDBObjectById(comment.getUserId(), null);
                  user.setText((userObject != null) ? userObject.getName() : i18n.tr(" <unknown>"));
                  user.layout();
               }
            });
         }
      });
		
		user.setImage(imageCache.create(ResourceManager.getImageDescriptor("icons/user.png"))); //$NON-NLS-1$
		user.setText((userObject != null) ? userObject.getName() : i18n.tr(" <unknown>"));
      user.setBackground(parent.getBackground());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = false;
		user.setLayoutData(gd);

      final Label time = new Label(this, SWT.NONE);
      time.setText(DateFormatFactory.getDateTimeFormat().format(comment.getLastChangeTime()));
      time.setBackground(parent.getBackground());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		time.setLayoutData(gd);

      final Composite controlArea = new Composite(this, SWT.NONE);
      controlArea.setBackground(parent.getBackground());
		layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		controlArea.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.verticalAlignment = SWT.TOP;
      gd.verticalSpan = 2;
      controlArea.setLayoutData(gd);

      if (enableEdit)
      {
         final ImageHyperlink linkEdit = new ImageHyperlink(controlArea, SWT.NONE);
         linkEdit.setText(i18n.tr("Edit"));
   		linkEdit.setImage(SharedIcons.IMG_EDIT);
         linkEdit.setBackground(parent.getBackground());
   		linkEdit.addHyperlinkListener(editAction);
   		gd = new GridData();
   		gd.horizontalAlignment = SWT.LEFT;
   		linkEdit.setLayoutData(gd);
   
         final ImageHyperlink linkDelete = new ImageHyperlink(controlArea, SWT.NONE);
         linkDelete.setText(i18n.tr("Delete"));
   		linkDelete.setImage(SharedIcons.IMG_DELETE_OBJECT);
         linkDelete.setBackground(parent.getBackground());
   		linkDelete.addHyperlinkListener(deleteAction);
         gd = new GridData();
         gd.horizontalAlignment = SWT.LEFT;
         linkDelete.setLayoutData(gd);
      }

		text = new Text(this, SWT.MULTI | SWT.WRAP | SWT.READ_ONLY);
		text.setText(comment.getText());
      text.setBackground(parent.getBackground());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		text.setLayoutData(gd);
	}
}
