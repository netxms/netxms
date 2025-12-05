/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.logwatch.dialogs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.dialogs.helpers.RuleListFilter;
import org.netxms.nxmc.modules.logwatch.dialogs.helpers.LogParserRuleLabelProvider;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParser;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserRule;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * LogParser rule selection dialog
 */
public class SelectLogParserRuleDlg extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SelectLogParserRuleDlg.class);
   private static final String CONFIG_PREFIX = "SelectLogRuleDlg";

   private LogParser logParserRulesCache;
   private String logName = "SyslogParser";
   private boolean multiSelection = true;
   private FilterText filterText;
   private TableViewer viewer;
   private RuleListFilter filter;
   private List<LogParserRule> selectedRules = new ArrayList<LogParserRule>();

   /**
    * @param parentShell
    * @param syslogLogParserCache
    */
   public SelectLogParserRuleDlg(Shell parentShell, LogParser logParserCache, String logName)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.logParserRulesCache = logParserCache;
      this.logName = logName;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Select Rule"));

      PreferenceStore settings = PreferenceStore.getInstance();
      int cx = settings.getAsInteger(CONFIG_PREFIX + ".cx", 600);
      int cy = settings.getAsInteger(CONFIG_PREFIX + ".cy", 460);
      newShell.setSize(cx, cy);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      PreferenceStore settings = PreferenceStore.getInstance();
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      filterText = new FilterText(dialogArea, SWT.NONE, null, false);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      filterText.setLayoutData(gd);
      final String filterString = settings.getAsString("SelectLogRuleDlg.Filter");
      if (filterString != null)
         filterText.setText(filterString);

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION | (multiSelection ? SWT.MULTI : SWT.SINGLE) | SWT.H_SCROLL | SWT.V_SCROLL);
      viewer.getTable().setLinesVisible(true);
      viewer.getTable().setHeaderVisible(true);

      TableColumn column = new TableColumn(viewer.getTable(), SWT.LEFT);
      column.setText("Name");
      column.setWidth(250);

      column = new TableColumn(viewer.getTable(), SWT.LEFT);
      column.setText("Match Expression");
      column.setWidth(250);

      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new LogParserRuleLabelProvider());
      filter = new RuleListFilter();
      if (filterString != null)
         filter.setFilterString(filterString);
      viewer.addFilter(filter);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 350;
      viewer.getTable().setLayoutData(gd);

      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            filter.setFilterString(filterText.getText());
            viewer.refresh();
         }
      });

      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            SelectLogParserRuleDlg.this.okPressed();
         }
      });

      if (logParserRulesCache == null)
      {
         viewer.getTable().setEnabled(false);
         getButton(IDialogConstants.OK_ID).setEnabled(false);
         final NXCSession session = Registry.getSession();
         Job job = new Job(i18n.tr("Get \"%1\" log parser rules", logName), null) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               try
               {
                  String content = session.getServerConfigClob(logName);
                  logParserRulesCache = LogParser.createFromXml(content);
               }
               catch(NXCException e)
               {
                  // If parser is not configured, server will return
                  // UNKNOWN_VARIABLE error
                  if (e.getErrorCode() != RCC.UNKNOWN_VARIABLE)
                     throw e;
                  logParserRulesCache = new LogParser();
               }
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.getTable().setEnabled(true);
                     getButton(IDialogConstants.OK_ID).setEnabled(true);
                     viewer.setInput(logParserRulesCache.getRules().toArray());
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot get \"%1\" log parser rules from server", logName);
            }
         };
         job.setUser(false);
         job.start();
      }
      else
      {
         viewer.setInput(logParserRulesCache.getRules().toArray());
      }

      return dialogArea;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
   @Override
   protected void cancelPressed()
   {
      saveSettings();
      super.cancelPressed();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @SuppressWarnings("unchecked")
   @Override
   protected void okPressed()
   {
      selectedRules = ((IStructuredSelection)viewer.getSelection()).toList();
      saveSettings();
      super.okPressed();
   }

   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      Point size = getShell().getSize();
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(CONFIG_PREFIX + ".cx", size.x);
      settings.set(CONFIG_PREFIX + ".cy", size.y);
      settings.set(CONFIG_PREFIX + ".Filter", filterText.getText());
   }

   /**
    * @return true if multiple event selection is enabled
    */
   public boolean isMultiSelectionEnabled()
   {
      return multiSelection;
   }

   /**
    * Enable or disable selection of multiple events.
    * 
    * @param enable true to enable multiselection, false to disable
    */
   public void enableMultiSelection(boolean enable)
   {
      this.multiSelection = enable;
   }

   /**
    * @return the selectedRules
    */
   public List<LogParserRule> getSelectedRules()
   {
      return selectedRules;
   }
}
