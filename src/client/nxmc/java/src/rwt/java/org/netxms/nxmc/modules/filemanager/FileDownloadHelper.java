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
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.netxms.client.AgentFileData;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.server.AgentFile;
import org.netxms.nxmc.DownloadServiceHandler;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.views.AgentFileManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Class to hide difference between RAP and RCP for file download
 */
public class FileDownloadHelper
{
   private static final Logger logger = LoggerFactory.getLogger(FileDownloadHelper.class);

   /**
    * Download file from agent
    * @param agentFileManager 
    */
   public static void startDownload(long objectId, IStructuredSelection selection, AgentFileManager agentFileManager)
   {
      NXCSession session = Registry.getSession();
      I18n i18n = LocalizationHelper.getI18n(FileDownloadHelper.class);
      for(Object o : selection.toList())
      {
         final AgentFile file = (AgentFile)o;
         if (!file.isDirectory())
         {
            downloadFile(session, objectId, file.getFullName(), file.getName());
         }
         else
         {      
            Job job = new Job("Download from agent", null) {
               @Override
               protected void run(final IProgressMonitor monitor) throws Exception
               {
                  long dirSize = -1;
                  try
                  {
                     dirSize = session.getAgentFileInfo(file).getSize();
                  }
                  catch(Exception e)
                  {
                  }
                  monitor.beginTask(String.format("Downloading directory %s", file.getName()), (int)dirSize);

                  final File zipFile = File.createTempFile("download_", ".zip");
                  FileOutputStream fos = new FileOutputStream(zipFile);
                  ZipOutputStream zos = new ZipOutputStream(fos);
                  downloadDir(session, objectId, file, file.getName(), zos, monitor);
                  zos.close();
                  fos.close();
                  if (!monitor.isCanceled())
                  {
                     DownloadServiceHandler.addDownload(zipFile.getName(), file.getName() + ".zip", zipFile, "application/octet-stream");
                     runInUIThread(new Runnable() {
                        @Override
                        public void run()
                        {
                           DownloadServiceHandler.startDownload(zipFile.getName());
                        }
                     });
                  }
                  monitor.done();
               }

               @Override
               protected String getErrorMessage()
               {
                  return i18n.tr("Cannot read remote directory");
               }
            };
            job.start();
         }
      }
   }
   
   /**
    * Download file from agent.
    * 
    * @param remoteName remote file name
    * @param localName local file name
    */
   private static void downloadFile(NXCSession session, long objectId, final String remoteName, final String localName)
   {
      final I18n i18n = LocalizationHelper.getI18n(FileDownloadHelper.class);
      logger.info("Start download of agent file " + remoteName);
      Job job = new Job(i18n.tr("Get server file list"), null) {
         @Override
         protected void run(final IProgressMonitor monitor) throws Exception
         {
            final AgentFileData file = session.downloadFileFromAgent(objectId, remoteName, 0, false, new ProgressListener() {
               @Override
               public void setTotalWorkAmount(long workTotal)
               {
                  monitor.beginTask("Downloading file " + remoteName, (int)workTotal);
               }

               @Override
               public void markProgress(long workDone)
               {
                  monitor.worked((int)workDone);
               }
            });

            if ((file != null) && (file.getFile() != null))
            {
               logger.info("Registering download with ID " + file.getId());
               DownloadServiceHandler.addDownload(file.getId(), localName, file.getFile(), "application/octet-stream");
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     DownloadServiceHandler.startDownload(file.getId());
                  }
               });
            }
            else
            {
               logger.info("Missing local file after download from agent (remote name " + remoteName + ")");
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
    * Recursively download directory from agent to local pc
    * 
    * @param sf
    * @param localFileName
    * @throws IOException 
    * @throws NXCException 
    */
   private static void downloadDir(NXCSession session, long objectId, final AgentFile sf, String localFileName, ZipOutputStream zos, final IProgressMonitor monitor) throws NXCException, IOException
   {
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
            downloadDir(session, objectId, f, localFileName + "/" + f.getName(), zos, monitor);
         }
         else
         {
            monitor.subTask(String.format("Compressing file %s", f.getFullName()));
            final AgentFileData file = session.downloadFileFromAgent(objectId, f.getFullName(), 0, false, new ProgressListener() {
               @Override
               public void setTotalWorkAmount(long workTotal)
               {
               }
               @Override
               public void markProgress(long workDone)
               {
                  monitor.worked((int)workDone);
               }
            });

            if(file != null && file.getFile() != null)
            {
               FileInputStream fis = new FileInputStream(file.getFile());
               ZipEntry zipEntry = new ZipEntry(localFileName + "/" + f.getName());
               zos.putNextEntry(zipEntry);
               byte[] bytes = new byte[1024];
               int length;
               while ((length = fis.read(bytes)) >= 0) 
               {
                  zos.write(bytes, 0, length);
               }
               zos.closeEntry();
               fis.close();
            }
         }
      }
   }

   /**
    * Upload local folder to agent
    * @param viewer 
    * @param agentFileManager 
    */
   public static void uploadFolder(long objectId, IStructuredSelection selection, AgentFileManager agentFileManager, SortableTreeViewer viewer)
   {
      //Do nothing
   }   
}
