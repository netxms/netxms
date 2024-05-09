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
package org.netxms.nxmc.modules.alarms.preferencepages;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.Clip;
import javax.sound.sampled.Line;
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
import org.eclipse.swt.widgets.Group;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.server.ServerFile;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.login.LoginJob;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.AlarmNotifier;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Alarm sound configuration
 */
public class AlarmSounds extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(AlarmSounds.class);
   private static Logger logger = LoggerFactory.getLogger(LoginJob.class);
   
   private NXCSession session;
   private ServerFile[] serverFiles = null;
   private File workspaceDir;
   private Button isGeneralSound;
   private Button isLocalSound;
   private PreferenceStore ps;
   private Set<String> soundList = new HashSet<String>();
   private List<String> currentSoundList = new ArrayList<String>();
   private Set<String> oldSoundList = new HashSet<String>();
   private List<String> newSoundList = new ArrayList<String>();
   private List<Combo> comboList = new ArrayList<Combo>();
   private List<Button> buttonList = new ArrayList<Button>();

   public AlarmSounds()
   {
      super(LocalizationHelper.getI18n(AlarmSounds.class).tr("Alarm sounds"));
      workspaceDir = Registry.getStateDir();
      ps = PreferenceStore.getInstance();
      setPreferenceStore(ps);
      session = Registry.getSession();
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
      layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      Group typeGroup = new Group(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 2;
      typeGroup.setLayout(layout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      typeGroup.setLayoutData(gd);
      
      isGeneralSound = new Button(typeGroup, SWT.RADIO);
      isGeneralSound.setText("Play sound on all alarms");
      isGeneralSound.setLayoutData(new GridData());
      isGeneralSound.setSelection(!ps.getAsBoolean("AlarmNotifier.LocalSound", false));
      
      isLocalSound = new Button(typeGroup, SWT.RADIO);
      isLocalSound.setText("Play sound as defined by active dashboard");
      isLocalSound.setLayoutData(new GridData());
      isLocalSound.setSelection(ps.getAsBoolean("AlarmNotifier.LocalSound", false));

      Combo newCombo = null;
      Button button = null;

      for(int i = 0; i < 6; i++)
      {
         final String soundId = (i < 5) ? StatusDisplayInfo.getStatusText(i) : "Outstanding alarm reminder";
         newCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, 
               soundId, WidgetHelper.DEFAULT_LAYOUT_DATA);
         newCombo.setEnabled(false);
         comboList.add(i, newCombo);
         button = new Button(dialogArea, SWT.PUSH);
         GridData gridData = new GridData();
         gridData.verticalAlignment = GridData.END;
         button.setLayoutData(gridData);
         button.setImage(SharedIcons.IMG_SOUND); 
         final int index = i; 
         button.addMouseListener(new MouseListener() {
            
            @Override
            public void mouseUp(MouseEvent e)
            {
               getMelodyAndDownloadIfRequired(comboList.get(index).getText());
               try
               {
                  Clip sound = (Clip) AudioSystem.getLine(new Line.Info(Clip.class));
                  sound.open(AudioSystem.getAudioInputStream(new File(workspaceDir.getPath(), comboList.get(index).getText()).getAbsoluteFile()));
                  sound.start();
                  while(!sound.isRunning())
                     Thread.sleep(10);
                  while(sound.isRunning())
                  {
                     Thread.sleep(10);
                  }
                  sound.close();
               }
               catch(final Exception ex)
               {
                  Display.getDefault().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        MessageDialogHelper.openError(Display.getDefault().getActiveShell(), 
                              i18n.tr("Was not possible to play sound"),
                              i18n.tr(" Error while playing sound. Melody will not be played. Error: ")+ ex.getMessage());
                     }
                  });
               }
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

      getShell().getDisplay().asyncExec(new Runnable() {
         
         @Override
         public void run()
         {
            try
            {
               String[] s = { "wav" }; //$NON-NLS-1$
               serverFiles = session.listServerFiles(s);
            }
            catch(final Exception e)
            {
               logger.error("Failed to list melody server files", e);
               getShell().getDisplay().asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     MessageDialogHelper.openError(getShell(), i18n.tr("Not possible to get melody list."),
                           i18n.tr(" Not possible to get melody list. Error: ")+ e.getMessage());
                  }
               });
            }

            for(ServerFile s : serverFiles)
            {
               soundList.add(s.getName());
            }
            soundList.add(""); //$NON-NLS-1$

            for(int i = 0; i < 6; i++)
            {
               currentSoundList.add(i, ps.getAsString("AlarmNotifier.Sound." + AlarmNotifier.SEVERITY_TEXT[i], "")); //$NON-NLS-1$
            }
            soundList.addAll(currentSoundList);

            Combo newCombo = null;

            for(int i = 0; i < 6; i++)
            {
               newCombo = comboList.get(i);
               newCombo.setEnabled(true);
               newCombo.setItems(soundList.toArray(new String[soundList.size()]));
               newCombo.select(newCombo.indexOf(currentSoundList.get(i)));
            }
         }
      });
      return dialogArea;
   }
   
   /**
    * @param severity
    * @return
    */
   private void getMelodyAndDownloadIfRequired(String melodyName)
   {
      if (!isSoundFileExist(melodyName, workspaceDir))
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
                  File f = new File(workspaceDir.getPath(), melodyName);
                  f.createNewFile();
                  dest = new FileOutputStream(f);
                  FileChannel fcSrc = src.getChannel();
                  dest.getChannel().transferFrom(fcSrc, 0, fcSrc.size());
               }
               catch(IOException e)
               {
                  logger.error("Cannot copy sound file", e);
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
                        i18n.tr("Melody does not exist."),
                        i18n.tr("Melody was not found locally and it was not possible to download it from server. Melody is removed and will not be played. Error: {0}",
                              e.getLocalizedMessage()));
               }
            });
         }
      }
   }

   /**
    * @param melodyName
    * @param workspaceDir2
    * @return
    */
   private static boolean isSoundFileExist(String melodyName, File workspaceDir2)
   {
      if (!melodyName.isEmpty() && (workspaceDir2 != null))
      {
         File f = new File(workspaceDir2.getPath(), melodyName);
         return f.isFile();
      }
      else
      {
         return true;
      }
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      if (isApply)
         setValid(false);

      newSoundList.clear();
      for(int i = 0; i < comboList.size(); i++)
      {
         newSoundList.add(comboList.get(i).getText());
      }
      final boolean newSelection = isGeneralSound.getSelection();
      getShell().getDisplay().asyncExec(new Runnable() {
         @Override
         public void run()
         {
            ps.set("AlarmNotifier.LocalSound", !newSelection); //$NON-NLS-1$
            for(int i = 0; i < newSoundList.size(); i++)
            {
               changeSound(newSoundList.get(i), AlarmNotifier.SEVERITY_TEXT[i], i);
            }
            for(String oldName : oldSoundList)
            {
               if (!currentSoundList.contains(oldName))
               {
                  File file = new File(workspaceDir.getPath(), oldName);
                  file.delete();
               }
            }
            if (isApply)
               setValid(true);            
         }
      });

      return true;
   }

   /**
    * Change sound for given alarm severity
    * 
    * @param soundName
    * @param severity
    * @param id
    */
   private void changeSound(final String soundName, final String severity, final int id)
   {
      String oldSoundName = currentSoundList.get(id);
      if (!soundName.equals(oldSoundName))
      {
         try
         {
            if (!checkMelodyExists(soundName, workspaceDir))
               downloadSoundFile(session, soundName, workspaceDir);
            ps.set("AlarmNotifier.Sound." + severity, soundName); //$NON-NLS-1$
            currentSoundList.set(id, soundName);
            oldSoundList.add(oldSoundName);
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
                              i18n.tr("Melody does not exist."),
                              i18n.tr("Melody was not found locally and it was not possible to download it from server. Melody is removed and will not be played. Error: {0}",
                                    e.getLocalizedMessage()));
               }
            });
            ps.set("ALARM_NOTIFIER.MELODY." + severity, ""); //$NON-NLS-1$ //$NON-NLS-2$
         }
      }
   }

   /**
    * @param melodyName
    * @param workspaceDir2
    * @return
    */
   private static boolean checkMelodyExists(String melodyName, File workspaceDir2)
   {
      if (workspaceDir2 != null && melodyName != null && !melodyName.equals("")) //$NON-NLS-1$
      {
         File f = new File(workspaceDir2.getPath(), melodyName);
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
    * @param workspaceDir2
    * @throws NXCException
    * @throws IOException
    */
   @SuppressWarnings("resource")
   private static void downloadSoundFile(NXCSession session, String melodyName, File workspaceDir2) throws NXCException, IOException
   {
      File serverFile = session.downloadFileFromServer(melodyName);
      FileChannel src = null;
      FileChannel dest = null;
      try
      {
         src = new FileInputStream(serverFile).getChannel();
         File f = new File(workspaceDir2.getPath(), melodyName);
         f.createNewFile();
         dest = new FileOutputStream(f).getChannel();
         dest.transferFrom(src, 0, src.size());
      }
      catch(IOException e)
      {
         logger.error("Cannot copy sound file", e);
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
