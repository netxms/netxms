/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.JavaScriptExecutor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.server.ServerFile;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.AlarmNotifier;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.console.DownloadServiceHandler;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Alarm sound configuration
 */
public class AlarmSounds extends PreferencePage implements IWorkbenchPreferencePage
{
   private NXCSession session;
   private ServerFile[] serverFiles = null;
   private IPreferenceStore ps;
   private URL workspaceUrl;
   private Set<String> melodyList = new HashSet<String>();
   private List<String> currentMelodyList = new ArrayList<String>();
   private Set<String> oldMelodyList = new HashSet<String>();
   private List<String> newMelodyList = new ArrayList<String>();
   private List<Combo> comboList = new ArrayList<Combo>();
   private List<Button> buttonList = new ArrayList<Button>();
   
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
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      Combo newCombo = null;
      Button button = null;

      for(int i = 0; i < 5; i++)
      {
         newCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, 
               StatusDisplayInfo.getStatusText(i), WidgetHelper.DEFAULT_LAYOUT_DATA);
         newCombo.setEnabled(false);
         comboList.add(i, newCombo);
         button = new Button(dialogArea, SWT.PUSH);
         GridData gridData = new GridData();
         gridData.verticalAlignment = GridData.END;
         button.setLayoutData(gridData);
         button.setImage(Activator.getImageDescriptor("icons/sound.png").createImage());
         final int index = i; 
         button.addMouseListener(new MouseListener() {
            
            @Override
            public void mouseUp(MouseEvent e)
            {
               String fileName = comboList.get(index).getText();
               getMelodyAndDownloadIfRequired(fileName);
               JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
               File localFile = new File(workspaceUrl.getPath(), fileName);
               String id = "audio-" + fileName;
               DownloadServiceHandler.addDownload(id, fileName, localFile, "audio/wav"); //$NON-NLS-1$
               StringBuilder js = new StringBuilder();
               js.append("var audio = new Audio('");//$NON-NLS-1$
               js.append(DownloadServiceHandler.createDownloadUrl(id));
               js.append("');");//$NON-NLS-1$
               js.append("audio.play();");//$NON-NLS-1$  
               executor.execute(js.toString());
            }
            
            @Override
            public void mouseDown(MouseEvent e)
            {
               //do noting
            }
            
            @Override
            public void mouseDoubleClick(MouseEvent e)
            {
               //do noting
            }
         });
         buttonList.add(i, button);
      }

      new UIJob(Messages.get().AlarmMelody_JobGetMelodyList) {
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
                     MessageDialogHelper.openError(getShell(), Messages.get().AlarmMelody_ErrorGettingMelodyList,
                           Messages.get().AlarmMelody_ErrorGettingMelodyListDescription + e.getMessage());
                  }
               });
            }

            for(ServerFile s : serverFiles)
            {
               melodyList.add(s.getName());
            }
            melodyList.add(""); //$NON-NLS-1$

            for(int i = 0; i < 5; i++)
            {
               currentMelodyList.add(i, ps.getString("ALARM_NOTIFIER.MELODY." + AlarmNotifier.severityArray[i])); //$NON-NLS-1$
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
    * @param severity
    * @return
    */
   private void getMelodyAndDownloadIfRequired(String melodyName)
   {
      URL workspaceUrl = Platform.getInstanceLocation().getURL();
      if (!isMelodyExists(melodyName, workspaceUrl))
      {
         try
         {
            File fileContent = session.downloadFileFromServer(melodyName);
            if (fileContent != null)
            {
               FileInputStream src = null;
               FileOutputStream dest = null;
               try
               {
                  src = new FileInputStream(fileContent);
                  File f = new File(workspaceUrl.getPath(), melodyName);
                  f.createNewFile();
                  dest = new FileOutputStream(f);
                  FileChannel fcSrc = src.getChannel();
                  dest.getChannel().transferFrom(fcSrc, 0, fcSrc.size());
               }
               catch(IOException e)
               {
                  Activator.logError("Cannot copy sound file", e); //$NON-NLS-1$
               }
               finally
               {
                  if (src != null)
                     src.close();
                  if (dest != null)
                     dest.close();
               }
            }
         }
         catch(final Exception e)
         {
            Display.getDefault().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  MessageDialogHelper
                  .openError(
                        Display.getDefault().getActiveShell(),
                        Messages.get().AlarmNotifier_ErrorMelodynotExists,
                        Messages.get().AlarmNotifier_ErrorMelodyNotExistsDescription
                              + e.getLocalizedMessage());
               }
            });
         }
      }
   }

   /**
    * @param melodyName
    * @param workspaceUrl
    * @return
    */
   private static boolean isMelodyExists(String melodyName, URL workspaceUrl)
   {
      if (!melodyName.isEmpty() && (workspaceUrl != null))
      {
         File f = new File(workspaceUrl.getPath(), melodyName);
         return f.isFile();
      }
      else
      {
         return true;
      }
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
      new UIJob(Messages.get().AlarmMelody_SaveClientSelection) {
         @Override
         public IStatus runInUIThread(IProgressMonitor monitor)
         {
            for(int i = 0; i < 5; i++)
            {
               changeMelody(newMelodyList.get(i), AlarmNotifier.severityArray[i], i);
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
               downloadSoundFile(session, melodyName, workspaceUrl);
            ps.setValue("ALARM_NOTIFIER.MELODY." + severity, melodyName); //$NON-NLS-1$
            currentMelodyList.set(id, melodyName);
            oldMelodyList.add(oldMelodyName);
         }
         catch(final Exception e)
         {
            getShell().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  MessageDialogHelper
                        .openError(
                              getShell(),
                              Messages.get().AlarmMelody_ErrorMelodyNotExists,
                              Messages.get().AlarmMelody_ErrorMelodyNotExistsDescription
                                    + e.getMessage());
               }
            });
            ps.setValue("ALARM_NOTIFIER.MELODY." + severity, ""); //$NON-NLS-1$ //$NON-NLS-2$
         }
      }
   }

   private static boolean checkMelodyExists(String melodyName, URL workspaceUrl)
   {
      if (workspaceUrl != null && melodyName != null && !melodyName.equals("")) //$NON-NLS-1$
      {
         File f = new File(workspaceUrl.getPath(), melodyName);
         return f.isFile();
      }
      else
      {
         return true;
      }
   }

   /**
    * Download sound file from server
    * 
    * @param session
    * @param melodyName
    * @param workspaceUrl
    * @throws NXCException
    * @throws IOException
    */
   @SuppressWarnings("resource")
   private static void downloadSoundFile(NXCSession session, String melodyName, URL workspaceUrl) throws NXCException, IOException
   {
      File serverFile = session.downloadFileFromServer(melodyName);
      FileChannel src = null;
      FileChannel dest = null;
      try
      {
         src = new FileInputStream(serverFile).getChannel();
         File f = new File(workspaceUrl.getPath(), melodyName);
         f.createNewFile();
         dest = new FileOutputStream(f).getChannel();
         dest.transferFrom(src, 0, src.size());
      }
      catch(IOException e)
      {
         Activator.logError("Cannot copy sound file", e); //$NON-NLS-1$
      }
      finally
      {
         if (src != null)
            src.close();
         if (dest != null)
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
