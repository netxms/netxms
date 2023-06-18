/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.base.preferencepages;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.FileDialog;
import org.netxms.nxmc.DownloadServiceHandler;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.dialogs.ThemeEditDialog;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.resources.DefaultDarkTheme;
import org.netxms.nxmc.resources.DefaultLightTheme;
import org.netxms.nxmc.resources.Theme;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Theme preferences
 */
public class ThemesPage extends PropertyPage
{
   private static final Logger logger = LoggerFactory.getLogger(ThemesPage.class);

   private Combo themeSelector;
   private Button importButton;
   private Button exportButton;
   private Button newButton;
   private Button editButton;
   private Button removeButton;
   private List<Theme> themes = new ArrayList<Theme>();

   /**
    * Create page.
    */
   public ThemesPage()
   {
      super("Themes");
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
      layout.numColumns = 6;
		dialogArea.setLayout(layout);

      loadThemes();

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 4;
      themeSelector = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY | SWT.BORDER, "Active theme", gd);
      updateThemeDropDown();

      String currentTheme = PreferenceStore.getInstance().getAsString("CurrentTheme");
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
            onThemeSelectionChange();
         }
      });

      editButton = new Button(dialogArea, SWT.PUSH);
      editButton.setText("&Edit...");
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(gd);
      editButton.setEnabled(themeSelector.getSelectionIndex() > 2);
      editButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editTheme();
         }
      });

      removeButton = new Button(dialogArea, SWT.PUSH);
      removeButton.setText("&Remove");
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      removeButton.setLayoutData(gd);
      removeButton.setEnabled(themeSelector.getSelectionIndex() > 2);
      removeButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            removeTheme();
         }
      });

      importButton = new Button(dialogArea, SWT.PUSH);
      importButton.setText("&Import...");
      gd = new GridData();
      gd.horizontalAlignment = SWT.CENTER;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      importButton.setLayoutData(gd);
      importButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            importTheme();
         }
      });

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

      exportButton = new Button(dialogArea, SWT.PUSH);
      exportButton.setText("E&xport...");
      gd = new GridData();
      gd.horizontalAlignment = SWT.CENTER;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      exportButton.setLayoutData(gd);
      exportButton.setEnabled(themeSelector.getSelectionIndex() > 2);
      exportButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            exportTheme();
         }
      });

		return dialogArea;
	}

   /**
    * Handle theme selection change
    */
   private void onThemeSelectionChange()
   {
      int index = themeSelector.getSelectionIndex();
      editButton.setEnabled(index > 2);
      removeButton.setEnabled(index > 2);
      exportButton.setEnabled(index > 2);
   }

   /**
    * Select theme programmatically.
    *
    * @param theme theme to select
    */
   private void selectTheme(Theme theme)
   {
      int index = themeSelector.indexOf(theme.getName());
      themeSelector.select((index != -1) ? index : 0);
      onThemeSelectionChange();
   }

   /**
    * Update dropdown with theme list
    */
   private void updateThemeDropDown()
   {
      themes.sort(new Comparator<Theme>() {
         @Override
         public int compare(Theme t1, Theme t2)
         {
            return t1.getName().compareToIgnoreCase(t2.getName());
         }
      });
      themeSelector.removeAll();
      themeSelector.add("[automatic]");
      themeSelector.add("Light [built-in]");
      themeSelector.add("Dark [built-in]");
      for(Theme t : themes)
         themeSelector.add(t.getName());
   }

   /**
    * Load custom themes from files
    */
   private void loadThemes()
   {
      File base = ThemeEngine.getThemeStorageDirectory();
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
               Theme t = Theme.load(f);
               t.setMissingElements(getAutomaticTheme());
               themes.add(t);
            }
            catch(Exception e)
            {
               logger.error("Error loading theme file " + f.getAbsolutePath(), e);
            }
         }
      }
   }

   /**
    * Add new theme
    */
   private void addNewTheme()
   {
      Theme currentTheme;
      int index = themeSelector.getSelectionIndex();
      switch(index)
      {
         case 0:
            currentTheme = getAutomaticTheme();
            break;
         case 1:
            currentTheme = new DefaultLightTheme();
            break;
         case 2:
            currentTheme = new DefaultDarkTheme();
            break;
         default:
            currentTheme = themes.get(index - 3);
            break;
      }
      Theme theme = new Theme("New Theme", currentTheme);
      ThemeEditDialog dlg = new ThemeEditDialog(getShell(), theme);
      if (dlg.open() == Window.OK)
      {
         try
         {
            ThemeEngine.saveTheme(theme);
            themes.add(theme);
            updateThemeDropDown();
            selectTheme(theme);
         }
         catch(Exception e)
         {
            MessageDialogHelper.openError(getShell(), "Error", String.format("Cannot save theme (%s)", e.getLocalizedMessage()));
            logger.error("Cannot save theme", e);
         }
      }
   }

   /**
    * Import theme from file
    */
   private void importTheme()
   {
      FileDialog dlg = new FileDialog(getShell(), SWT.OPEN);
      dlg.setFilterExtensions(new String[] { "*.xml", "*.*" });
      String importFileName = dlg.open();
      if (importFileName == null)
         return;

      try
      {
         Theme theme = Theme.load(new File(importFileName));
         ThemeEngine.saveTheme(theme);
         themes.add(theme);
         updateThemeDropDown();
         selectTheme(theme);
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getShell(), "Error", String.format("Cannot import theme (%s)", e.getLocalizedMessage()));
         logger.error("Cannot import theme", e);
      }
   }

   /**
    * Edit currently selected theme
    */
   private void editTheme()
   {
      int index = themeSelector.getSelectionIndex();
      if (index < 3)
         return;

      Theme theme = themes.get(index - 3);
      String oldName = theme.getName();
      ThemeEditDialog dlg = new ThemeEditDialog(getShell(), theme);
      if (dlg.open() == Window.OK)
      {
         try
         {
            if (!oldName.equals(theme.getName()))
               ThemeEngine.deleteTheme(oldName);
            ThemeEngine.saveTheme(theme);
            updateThemeDropDown();
            selectTheme(theme);
         }
         catch(Exception e)
         {
            MessageDialogHelper.openError(getShell(), "Error", String.format("Cannot save theme (%s)", e.getLocalizedMessage()));
            logger.error("Cannot save theme", e);
         }
      }
   }

   /**
    * Remove currently selected theme
    */
   private void removeTheme()
   {
      int index = themeSelector.getSelectionIndex();
      if (index < 3)
         return;

      Theme theme = themes.get(index - 3);
      if (!MessageDialogHelper.openQuestion(getShell(), "Remove Theme", String.format("Theme %s will be removed. Are you sure?", theme.getName())))
         return;

      ThemeEngine.deleteTheme(theme.getName());
      updateThemeDropDown();
      themeSelector.select(0);
      onThemeSelectionChange();
   }

   /**
    * Export currently selected theme
    */
   private void exportTheme()
   {
      int index = themeSelector.getSelectionIndex();
      if (index < 3)
         return;

      final Theme theme = themes.get(index - 3);
      new Job("Export theme", null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final File tmpFile = File.createTempFile("ExportTheme", "_" + System.currentTimeMillis());
            theme.save(tmpFile);

            DownloadServiceHandler.addDownload(tmpFile.getName(), theme.getName() + ".xml", tmpFile, "text/xml");
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  DownloadServiceHandler.startDownload(tmpFile.getName());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot export theme";
         }
      }.start();
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
      removeButton.setEnabled(false);
      exportButton.setEnabled(false);
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      PreferenceStore.getInstance().set("CurrentTheme", themeSelector.getText());
      ThemeEngine.reload();
      return true;
   }

   /**
    * Get automatic theme.
    *
    * @return automatically created built-in theme
    */
   private static Theme getAutomaticTheme()
   {
      return WidgetHelper.isSystemDarkTheme() ? new DefaultDarkTheme() : new DefaultLightTheme();
   }
}
