/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console.preferencepages;

import java.io.File;
import java.io.FilenameFilter;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.window.Window;
import org.eclipse.osgi.service.datalocation.Location;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.dialogs.ThemeEditDialog;
import org.netxms.ui.eclipse.console.resources.DefaultLightTheme;
import org.netxms.ui.eclipse.console.resources.Theme;
import org.netxms.ui.eclipse.console.resources.ThemeEngine;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Theme preferences
 */
public class ThemePrefs extends PreferencePage implements IWorkbenchPreferencePage
{
   private Combo themeSelector;
   private Button importButton;
   private Button newButton;
   private Button editButton;
   List<Theme> themes = new ArrayList<Theme>();

   /**
    * @see org.eclipse.ui.IWorkbenchPreferencePage#init(org.eclipse.ui.IWorkbench)
    */
	@Override
	public void init(IWorkbench workbench)
	{
		setPreferenceStore(Activator.getDefault().getPreferenceStore());
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 3;
		dialogArea.setLayout(layout);

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      themeSelector = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY | SWT.BORDER, "Theme", gd);
      themeSelector.add("Automatic");
      themeSelector.add("Default Light");
      themeSelector.add("Default Dark");

      loadThemes();
      for(Theme t : themes)
         themeSelector.add(t.getName());

      String currentTheme = getPreferenceStore().getString("CurrentTheme");
      if ((currentTheme != null) && !currentTheme.isEmpty())
      {
         int index = themeSelector.indexOf(currentTheme);
         themeSelector.select((index != -1) ? index : 0);
      }
      else
      {
         themeSelector.select(0);
      }
      themeSelector.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editButton.setEnabled(themeSelector.getSelectionIndex() > 2);
         }
      });

      importButton = new Button(dialogArea, SWT.PUSH);
      importButton.setText("&Import...");
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.RIGHT;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      importButton.setLayoutData(gd);

      newButton = new Button(dialogArea, SWT.PUSH);
      newButton.setText("&New...");
      gd = new GridData();
      gd.horizontalAlignment = SWT.CENTER;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      newButton.setLayoutData(gd);
      newButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addNewTheme();
         }
      });

      editButton = new Button(dialogArea, SWT.PUSH);
      editButton.setText("&Edit...");
      gd = new GridData();
      gd.horizontalAlignment = SWT.CENTER;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(gd);
      editButton.setEnabled(themeSelector.getSelectionIndex() > 2);

		return dialogArea;
	}

   /**
    * Load custom themes from files
    */
   private void loadThemes()
   {
      Location loc = Platform.getInstanceLocation();
      File targetDir;
      try
      {
         targetDir = new File(loc.getURL().toURI());
      }
      catch(URISyntaxException e)
      {
         targetDir = new File(loc.getURL().getPath());
      }
      File base = new File(targetDir, "themes");
      if (base.isDirectory())
      {
         for(File f : base.listFiles(new FilenameFilter() {
            @Override
            public boolean accept(File dir, String name)
            {
               return name.endsWith(".xml");
            }
         }))
         {
            try
            {
               themes.add(Theme.load(f));
            }
            catch(Exception e)
            {
               Activator.logError("Error loading theme file " + f.getAbsolutePath(), e);
            }
         }
      }

      themes.sort(new Comparator<Theme>() {
         @Override
         public int compare(Theme t1, Theme t2)
         {
            return t1.getName().compareToIgnoreCase(t2.getName());
         }
      });
   }

   /**
    * Add new theme
    */
   private void addNewTheme()
   {
      Theme theme = new Theme("New Theme", new DefaultLightTheme());
      ThemeEditDialog dlg = new ThemeEditDialog(getShell(), theme);
      if (dlg.open() == Window.OK)
      {
      }
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
      themeSelector.select(0);
      editButton.setEnabled(false);
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
      getPreferenceStore().setValue("CurrentTheme", themeSelector.getText());
      ThemeEngine.reload();
		return super.performOk();
	}
}
