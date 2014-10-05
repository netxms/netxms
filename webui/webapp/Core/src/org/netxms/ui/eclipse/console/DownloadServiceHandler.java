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
package org.netxms.ui.eclipse.console;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.service.ServiceHandler;

/**
 * Service handler for downloading files
 */
public class DownloadServiceHandler implements ServiceHandler
{
	private static Map<String, DownloadInfo> downloads = new HashMap<String, DownloadInfo>();

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.rap.rwt.service.ServiceHandler#service(javax.servlet.http.HttpServletRequest,
	 * javax.servlet.http.HttpServletResponse)
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
			response.sendError(404);
			return;
		}

		// Send the file in the response
		response.setContentType(info.contentType);
		response.setHeader("Content-Disposition", "attachment; filename=\"" + info.name + "\"");
		if (info.localFile != null)
		{
	      response.setContentLength((int)info.localFile.length());
			InputStream in = new FileInputStream(info.localFile);
			try
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
			finally
			{
				in.close();
			}
			
			// remove file after successful download
			synchronized(downloads)
			{
				downloads.remove(id);
				info.localFile.delete();
			}
		}
		else
		{
			response.setContentLength(info.data.length);
			response.getOutputStream().write(info.data);
		}
	}

	/**
	 * @param name
	 * @return
	 */
	public static String createDownloadUrl(String id)
	{
		StringBuilder url = new StringBuilder(RWT.getServiceManager().getServiceHandlerUrl("downloadServiceHandler"));
		url.append("&id=");
		url.append(id);
		return RWT.getResponse().encodeURL(url.toString());
	}

	/**
	 * @param name
	 * @param localFile
	 * @param contentType
	 */
	public static void addDownload(String id, String name, File localFile, String contentType)
	{
		synchronized(downloads)
		{
			downloads.put(id, new DownloadInfo(name, localFile, contentType, null));
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
			downloads.put(id, new DownloadInfo(name, null, contentType, data));
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

		DownloadInfo(String name, File localFile, String contentType, byte[] data)
		{
			this.name = name;
			this.localFile = localFile;
			this.contentType = contentType;
			this.data = data;
		}
	}
}
