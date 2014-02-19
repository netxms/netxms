/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.alarmviewer.preferencepages;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URL;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerFile;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.AlarmNotifier;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Alarm melody
 */
public class AlarmMelody extends PreferencePage implements IWorkbenchPreferencePage
{
   public static final String ID = "org.netxms.ui.eclipse.alarmviewer.views.AlarmMelodyView"; //$NON-NLS-1$

   private NXCSession session;
   private ServerFile[] serverFiles = null;
   private IPreferenceStore ps;
   private URL workspaceUrl;
   private Set<String> melodyList = new HashSet<String>();
   private List<String> currentMelodyList = new ArrayList<String>();
   private Set<String> oldMelodyList = new HashSet<String>();
   private List<String> newMelodyList = new ArrayList<String>();
   private List<Combo> comboList = new ArrayList<Combo>();
   private String[] severityArray = AlarmNotifier.severityArray;
   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.IWorkbenchPreferencePage#init(org.eclipse.ui.IWorkbench)
    */
   @Override
   public void init(IWorkbench workbench)
   {
      setPreferenceStore(Activator.getDefault().getPreferenceStore());
      session = (NXCSession)ConsoleSharedData.getSession();
      ps = Activator.getDefault().getPreferenceStore();
      workspaceUrl = Platform.getInstanceLocation().getURL();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 1;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      Combo newCombo = null;

      for(int i = 0; i < 5; i++)
      {
         newCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, StatusDisplayInfo.getStatusText(i),
               WidgetHelper.DEFAULT_LAYOUT_DATA);
         newCombo.setEnabled(false);
         comboList.add(i, newCombo);
      }

      new UIJob("Start alarm melody save") {
         @Override
         public IStatus runInUIThread(IProgressMonitor monitor)
         {
            try
            {
               String[] s = { "wav" }; //$NON-NLS-1$
               serverFiles = session.listServerFiles(s);
            }
            catch(final Exception e)
            {
               e.printStackTrace();
               getShell().getDisplay().asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     MessageDialogHelper.openError(getShell(), "Not possible to get melody list.",
                           "Not possible to get melody list. Error: " + e.getMessage());
                  }
               });
            }

            for(ServerFile s : serverFiles)
            {
               melodyList.add(s.getName());
            }
            melodyList.add("");

            for(int i = 0; i < 5; i++)
            {
               currentMelodyList.add(i, ps.getString("ALARM_NOTIFIER.MELODY." + severityArray[i]));
            }
            melodyList.addAll(currentMelodyList);

            Combo newCombo = null;

            for(int i = 0; i < 5; i++)
            {
               newCombo = comboList.get(i);
               newCombo.setEnabled(true);
               newCombo.setItems(melodyList.toArray(new String[melodyList.size()]));
               newCombo.select(newCombo.indexOf(currentMelodyList.get(i)));
            }

            return Status.OK_STATUS;
         }
      }.schedule();
      return dialogArea;
   }

   /**
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   protected void applyChanges(final boolean isApply)
   {
      if (isApply)
         setValid(false);

      for(int i = 0; i < 5; i++)
      {
         newMelodyList.add(comboList.get(i).getText());
      }
      new UIJob("Start alarm melody save") {
         @Override
         public IStatus runInUIThread(IProgressMonitor monitor)
         {
            for(int i = 0; i < 5; i++)
            {
               changeMelody(newMelodyList.get(i), severityArray[i], i);
            }
            for(String oldName : oldMelodyList)
            {
               if (!currentMelodyList.contains(oldName))
               {
                  File file = new File(workspaceUrl.getPath(), oldName);
                  file.delete();
               }
            }
            if (isApply)
               setValid(true);
            return Status.OK_STATUS;
         }

      }.schedule();
   }

   private void changeMelody(final String melodyName, final String severity, final int id)
   {
      String oldMelodyName = currentMelodyList.get(id);
      if (!melodyName.equals(oldMelodyName))
      {
         try
         {
            if (!checkMelodyExists(melodyName, workspaceUrl))
               downloadMelodie(session, melodyName, workspaceUrl);
            ps.setValue("ALARM_NOTIFIER.MELODY." + severity, melodyName);
            currentMelodyList.set(id, melodyName);
            oldMelodyList.add(oldMelodyName);
         }
         catch(NXCException | IOException e)
         {
            getShell().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  MessageDialogHelper
                        .openError(
                              getShell(),
                              "Melody does not exist.",
                              "Melody was not found "
                                    + "locally and it was not possible to download it from server. Melody is removed and will not be played. Error: "
                                    + e.getMessage());
               }
            });
            ps.setValue("ALARM_NOTIFIER.MELODY." + severity, "");
         }
      }
   }

   private static boolean checkMelodyExists(String melodyName, URL workspaceUrl)
   {
      if (workspaceUrl != null && melodyName != null && !melodyName.equals(""))
      {
         File f = new File(workspaceUrl.getPath(), melodyName);
         return f.isFile();
      }
      else
      {
         return true;
      }
   }

   private static void downloadMelodie(NXCSession session, String melodyName, URL workspaceUrl) throws NXCException, IOException
   {
      File f = new File(workspaceUrl.getPath(), melodyName);
      f.createNewFile();
      FileChannel src = null;
      FileChannel dest = null;
      try
      {
         src = new FileInputStream(session.downloadFileFromServer(melodyName)).getChannel();
         dest = new FileOutputStream(f).getChannel();
         dest.transferFrom(src, 0, src.size());
      }
      catch(NXCException | IOException e)
      {
         System.out.println("Not possible to copy inside new file content.");
      }
      finally
      {
         src.close();
         dest.close();
      }
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      applyChanges(false);
      return true;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      applyChanges(true);
   }
}
