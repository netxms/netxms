/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayDeque;
import java.util.Deque;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.JavaScriptExecutor;
import org.eclipse.rap.rwt.service.ServiceHandler;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import jakarta.servlet.ServletException;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;

/**
 * Service handler for downloading files
 */
public class DownloadServiceHandler implements ServiceHandler
{
   private static final Logger logger = LoggerFactory.getLogger(DownloadServiceHandler.class);

   public static final String ID = "downloadServiceHandler";

	private static Map<String, DownloadInfo> downloads = new HashMap<String, DownloadInfo>();
	private static Deque<String> iframeIds = new ArrayDeque<String>(16);

	/**
	 * @see org.eclipse.rap.rwt.service.ServiceHandler#service(javax.servlet.http.HttpServletRequest, javax.servlet.http.HttpServletResponse)
	 */
	@Override
	public void service(HttpServletRequest request, HttpServletResponse response) throws IOException, ServletException
	{
		String id = request.getParameter("id");
		DownloadInfo info;
		synchronized(downloads)
		{
			info = downloads.get(id);
		}

		if (info == null)
		{
         logger.debug("Cannot find download with ID {}", id);
			response.sendError(404);
			return;
		}

		// Send the file in the response
		response.setContentType(info.contentType);
      if (info.attachment)
         response.setHeader("Content-Disposition", "attachment; filename=\"" + info.name + "\"");

		if (info.localFile != null)
		{
	      response.setContentLength((int)info.localFile.length());
         try (InputStream in = new FileInputStream(info.localFile))
			{
				OutputStream out = response.getOutputStream();

				byte[] buffer = new byte[8192];
				int len = in.read(buffer);
				while(len != -1)
				{
					out.write(buffer, 0, len);
               response.flushBuffer();
					len = in.read(buffer);
				}
			}
		}
		else
		{
			response.setContentLength(info.data.length);
			response.getOutputStream().write(info.data);
		}

      // remove file after successful download
      logger.debug("Download with ID {} completed", id);

      if (!info.persistent)
      {
         synchronized(downloads)
         {
            downloads.remove(id);
            if (info.localFile != null)
               info.localFile.delete();
            iframeIds.push(info.iframeId);
         }
      }
	}

	/**
	 * @param name
	 * @return
	 */
	public static String createDownloadUrl(String id)
	{
      StringBuilder url = new StringBuilder(RWT.getServiceManager().getServiceHandlerUrl(ID));
		url.append("&id=");
		url.append(id);
		return RWT.getResponse().encodeURL(url.toString());
	}

	/**
    * @param id
    * @param name
    * @param localFile
    * @param contentType
    */
	public static void addDownload(String id, String name, File localFile, String contentType)
	{
		synchronized(downloads)
		{
         downloads.put(id, new DownloadInfo(name, localFile, contentType, null, false, true));
		}
	}

   /**
    * @param id
    * @param name
    * @param localFile
    * @param contentType
    */
   public static void addDownload(String id, String name, File localFile, String contentType, boolean persistent, boolean attachment)
   {
      synchronized(downloads)
      {
         downloads.put(id, new DownloadInfo(name, localFile, contentType, null, persistent, attachment));
      }
   }

	/**
	 * @param name
	 * @param data
	 * @param contentType
	 */
	public static void addDownload(String id, String name, byte[] data, String contentType)
	{
		synchronized(downloads)
		{
         downloads.put(id, new DownloadInfo(name, null, contentType, data, false, true));
		}
	}

   /**
    * Remove download.
    *
    * @param id download ID
    */
   public static void removeDownload(String id)
   {
      synchronized(downloads)
      {
         DownloadInfo info = downloads.remove(id);
         if (info != null)
         {
            iframeIds.push(info.iframeId);
            if (info.localFile != null)
               info.localFile.delete();
            logger.debug("Download with ID {} removed", id);
         }
      }
   }

	/**
	 * Start download that was added previously
	 * 
	 * @param id
	 */
	public static void startDownload(String id)
	{
      JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
      if (executor != null) 
      {
         StringBuilder js = new StringBuilder();
         js.append("var hiddenIFrameID = 'hiddenDownloader_");
         js.append(getIFrameId(id));
         js.append("', iframe = document.getElementById(hiddenIFrameID);");
         js.append("if (iframe === null) {");
         js.append("   iframe = document.createElement('iframe');");
         js.append("   iframe.id = hiddenIFrameID;");
         js.append("   iframe.style.display = 'none';");
         js.append("   document.body.appendChild(iframe);");
         js.append("}");
         js.append("iframe.src = '");
         js.append(DownloadServiceHandler.createDownloadUrl(id));
         js.append("';");
         executor.execute(js.toString());
      }                 
	}

	/**
	 * Get iframe ID for given download ID
	 *
	 * @param id download ID
	 * @return iframe ID
	 */
	private static String getIFrameId(String id)
	{
      synchronized(downloads)
      {
         DownloadInfo info = downloads.get(id);
         return (info != null) ? info.iframeId : "";
      }
	}

	/**
	 * Download information
	 */
	private static class DownloadInfo
	{
		String name;
		File localFile; // if null, use in-memory data
		String contentType;
		byte[] data;
		String iframeId;
      boolean persistent;
      boolean attachment;

      DownloadInfo(String name, File localFile, String contentType, byte[] data, boolean persistent, boolean attachment)
		{
			this.name = name;
			this.localFile = localFile;
			this.contentType = contentType;
			this.data = data;
			this.iframeId = iframeIds.isEmpty() ? UUID.randomUUID().toString() : iframeIds.pop();
         this.persistent = persistent;
         this.attachment = attachment;
		}
	}
}
