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
package org.netxms.nxmc.modules.incidents.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.events.IncidentComment;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Display widget for incident comments
 */
public class IncidentCommentsEditor extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(IncidentCommentsEditor.class);

   private NXCSession session = Registry.getSession();
   private Text text;

   /**
    * Create incident comment display widget.
    *
    * @param parent parent composite
    * @param imageCache image cache for icons
    * @param comment the comment to display
    */
   public IncidentCommentsEditor(Composite parent, IncidentComment comment)
   {
      super(parent, SWT.NONE);

      setBackground(parent.getBackground());

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginWidth = 0;
      setLayout(layout);

      final CLabel user = new CLabel(this, SWT.NONE);
      final AbstractUserObject userObject = session.findUserDBObjectById(comment.getUserId(), () -> {
         parent.getDisplay().asyncExec(() -> {
            if (!user.isDisposed())
            {
               final AbstractUserObject u = session.findUserDBObjectById(comment.getUserId(), null);
               if (u != null)
               {
                  user.setText(u.getName());
                  user.getParent().layout(true, true);
               }
            }
         });
      });

      user.setImage(SharedIcons.IMG_USER);
      user.setText((userObject != null) ? userObject.getName() : i18n.tr("<unknown>"));
      user.setBackground(parent.getBackground());
      user.setForeground(getDisplay().getSystemColor(SWT.COLOR_LINK_FOREGROUND));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = false;
      user.setLayoutData(gd);

      final Label time = new Label(this, SWT.NONE);
      time.setText(DateFormatFactory.getDateTimeFormat().format(comment.getCreationTime()));
      time.setBackground(parent.getBackground());
      time.setForeground(getDisplay().getSystemColor(SWT.COLOR_WIDGET_DISABLED_FOREGROUND));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      time.setLayoutData(gd);

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
