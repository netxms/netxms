/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Reden Solutions
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
package org.netxms.nxmc.modules.filemanager;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.swt.widgets.FileDialog;
import org.netxms.client.AgentFileData;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.server.AgentFile;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.dialogs.StartClientToAgentFolderUploadDialog;
import org.netxms.nxmc.modules.filemanager.views.AgentFileManager;
import org.xnap.commons.i18n.I18n;

/**
 * Class to hide difference between RAP and RCP for file download
 */
public class FileDownloadHelper
{
   private static final I18n i18n = LocalizationHelper.getI18n(FileDownloadHelper.class);

   /**
    * Download file from agent
    */
   public static void startDownload(long objectId, IStructuredSelection selection, AgentFileManager view)
   {
      NXCSession session = Registry.getSession();
      AgentFile sf = (AgentFile)selection.getFirstElement();

      final String target;
      if (!sf.isDirectory() && (selection.size() == 1))
      {
         FileDialog dlg = new FileDialog(view.getWindow().getShell(), SWT.SAVE);
         dlg.setText(i18n.tr("Start Download"));
         String[] filterExtensions = { "*.*" }; 
         dlg.setFilterExtensions(filterExtensions);
         String[] filterNames = { i18n.tr("All Files") };
         dlg.setFilterNames(filterNames);
         dlg.setFileName(sf.getName());
         dlg.setOverwrite(true);
         target = dlg.open();
      }
      else
      {
         DirectoryDialog dlg = new DirectoryDialog(view.getWindow().getShell());
         target = dlg.open();
      }

      if (target == null)
         return;

      final List<AgentFile> files = new ArrayList<AgentFile>(selection.size());
      for(Object o : selection.toList())
         files.add((AgentFile)o);

      Job job = new Job("Download from agent", view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if (files.size() == 1)
            {
               AgentFile f = files.get(0);
               if (f.isDirectory())
               {
                  long dirSize = -1;
                  try
                  {
                     dirSize = session.getAgentFileInfo(f).getSize();
                  }
                  catch(Exception e)
                  {
                  }
                  monitor.beginTask(String.format("Downloading directory %s", f.getName()), (dirSize >= 0) ? (int)dirSize : IProgressMonitor.UNKNOWN);
                  downloadDir(session, objectId, f, target + "/" + f.getName(), monitor);
                  monitor.done();
               }
               else
               {
                  downloadFile(session, objectId, f, target, monitor, false);
               }
            }
            else
            {
               long total = 0;
               for(AgentFile f : files)
               {
                  if (monitor.isCanceled())
                     break;
                  if (f.isDirectory() && (f.getSize() < 0))
                  {
                     try
                     {
                        total += session.getAgentFileInfo(f).getSize();
                     }
                     catch(Exception e)
                     {
                     }
                  }
                  else
                  {
                     total += f.getSize();
                  }
               }
               
               monitor.beginTask("Downloading files", (int)total);
               for(AgentFile f : files)
               {
                  if (monitor.isCanceled())
                     break;
                  if (f.isDirectory())
                  {
                     downloadDir(session, objectId, f, target + "/" + f.getName(), monitor);
                  }
                  else
                  {
                     downloadFile(session, objectId, f, target + "/" + f.getName(), monitor, true);
                  }
               }
               monitor.done();
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Download from agent failed";
         }
      };
      job.start();
   }
   
   /**
    * Download file from agent.
    * 
    * @param remoteName remote file name
    * @param localName local file name
    */
   private static void downloadFile(NXCSession session, long objectId, final AgentFile sf, final String localName, final IProgressMonitor monitor, final boolean subTask)
         throws IOException, NXCException
   {
      if (subTask)
         monitor.subTask(String.format("Downloading file %s", sf.getFullName()));
      final AgentFileData file = session.downloadFileFromAgent(objectId, sf.getFullName(), 0, false, new ProgressListener() {
         @Override
         public void setTotalWorkAmount(long workTotal)
         {
            if (!subTask)
               monitor.beginTask(String.format("Downloading file %s", sf.getFullName()), (int)workTotal);
         }

         @Override
         public void markProgress(long workDone)
         {
            monitor.worked((int)workDone);
         }
      });
      if (file.getFile() != null)
      {
         File outputFile = new File(localName);
         outputFile.createNewFile();
         InputStream in = new FileInputStream(file.getFile());
         OutputStream out = new FileOutputStream(outputFile);
         byte[] buf = new byte[1024];
         int len;
         while((len = in.read(buf)) > 0)
         {
            out.write(buf, 0, len);
         }
         in.close();
         out.close();
         outputFile.setLastModified(sf.getModificationTime().getTime());
      }
   }

   /**
    * Recursively download directory from agent to local pc
    * 
    * @param sf
    * @param localFileName
    * @throws IOException 
    * @throws NXCException 
    */
   private static void downloadDir(NXCSession session, long objectId, final AgentFile sf, String localFileName, final IProgressMonitor monitor) throws NXCException, IOException
   {
      File dir = new File(localFileName);
      dir.mkdir();
      List<AgentFile> files = sf.getChildren();
      if (files == null)
      {
         files = session.listAgentFiles(sf, sf.getFullName(), sf.getNodeId());
         sf.setChildren(files);
      }
      for(AgentFile f : files)
      {
         if (monitor.isCanceled())
            break;
         if (f.isDirectory())
         {
            downloadDir(session, objectId, f, localFileName + "/" + f.getName(), monitor);
         }
         else
         {
            downloadFile(session, objectId, f, localFileName + "/" + f.getName(), monitor, true);
         }
      }
      dir.setLastModified(sf.getModificationTime().getTime());
   }

   /**
    * Upload local folder to agent
    */
   public static void uploadFolder(long objectId, IStructuredSelection selection, AgentFileManager view, final SortableTreeViewer viewer)
   {
      final Object[] objects = selection.toArray();
      final AgentFile uploadFolder = ((AgentFile)objects[0]).isDirectory() ? ((AgentFile)objects[0]) : ((AgentFile)objects[0]).getParent();

      final StartClientToAgentFolderUploadDialog dlg = new StartClientToAgentFolderUploadDialog(view.getWindow().getShell());
      if (dlg.open() == Window.OK)
      {
         Job job = new UploadConsoleJob(i18n.tr("Upload folder to agent"), dlg.getLocalFile(), objectId, uploadFolder, dlg.getRemoteFileName(), view, viewer);
         job.start();
      }
   }
}
