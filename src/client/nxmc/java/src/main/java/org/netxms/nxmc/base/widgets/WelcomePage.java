/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.base.VersionInfo;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.resources.SharedIcons;

/**
 * Welcome page
 */
public class WelcomePage extends Composite
{
   /**
    * @param parent
    * @param style
    */
   public WelcomePage(Composite parent, int style)
   {
      super(parent, style);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      setLayout(layout);

      Composite header = new Composite(this, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 2;
      header.setLayout(layout);
      header.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Label title = new Label(header, SWT.NONE);
      title.setText("Welcome to NetXMS " + VersionInfo.baseVersion());
      title.setFont(JFaceResources.getBannerFont());
      title.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      ToolBar toolbar = new ToolBar(header, SWT.FLAT | SWT.HORIZONTAL | SWT.RIGHT);
      toolbar.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

      Label separator = new Label(this, SWT.SEPARATOR | SWT.HORIZONTAL);
      separator.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      ToolItem closeButton = new ToolItem(toolbar, SWT.PUSH);
      closeButton.setImage(SharedIcons.IMG_CLOSE);
      closeButton.setText("Close");
      closeButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            WelcomePage.this.dispose();
            PreferenceStore.getInstance().set("WelcomePage.LastDisplayedVersion", VersionInfo.baseVersion());
         }
      });

      Browser browser = new Browser(this, SWT.NONE);
      browser.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      browser.setUrl("https://netxms.com/release-notes");
   }
}
