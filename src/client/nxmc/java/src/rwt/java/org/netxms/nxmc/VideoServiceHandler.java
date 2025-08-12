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
import java.io.IOException;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.service.ServiceHandler;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import jakarta.servlet.ServletException;
import jakarta.servlet.ServletOutputStream;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;

/**
 * Service handler for serving video files
 */
public class VideoServiceHandler implements ServiceHandler
{
   private static final Logger logger = LoggerFactory.getLogger(VideoServiceHandler.class);

   public static final String ID = "videoServiceHandler";

   private static final String MULTIPART_BOUNDARY = "MULTIPART_NEXT_PART";

	private static Map<String, File> videoFiles = new HashMap<String, File>();

	/**
	 * @see org.eclipse.rap.rwt.service.ServiceHandler#service(javax.servlet.http.HttpServletRequest, javax.servlet.http.HttpServletResponse)
	 */
	@Override
	public void service(HttpServletRequest request, HttpServletResponse response) throws IOException, ServletException
	{
		String id = request.getParameter("id");
		File vf;
		synchronized(videoFiles)
		{
			vf = videoFiles.get(id);
		}

		if (vf == null)
		{
         logger.error("Cannot find video file with ID {}", id);
			response.sendError(HttpServletResponse.SC_NOT_FOUND);
			return;
		}
      int contentLength = (int)vf.length();

      // Validate and process Range header
      List<Range> ranges = new ArrayList<Range>();
      String range = request.getHeader("Range");
      if (range != null)
      {
         // Range header should match format "bytes=n-n,n-n,n-n...". If not, then return 416.
         if (!range.matches("^bytes=\\d*-\\d*(,\\d*-\\d*)*$"))
         {
            response.setHeader("Content-Range", "bytes */" + contentLength); // Required in 416.
            response.sendError(HttpServletResponse.SC_REQUESTED_RANGE_NOT_SATISFIABLE);
            return;
         }

         for(String part : range.substring(6).split(","))
         {
            String[] s = part.split("-");
            if (s.length != 2)
               continue;
            
            long start = s[0].isEmpty() ? 0 : Long.parseLong(s[0]);
            long end = s[1].isEmpty() ? (contentLength - 1) : Long.parseLong(s[1]);

            if (start > end)
            {
               response.setHeader("Content-Range", "bytes */" + contentLength); // Required in 416.
               response.sendError(HttpServletResponse.SC_REQUESTED_RANGE_NOT_SATISFIABLE);
               return;
            }

            ranges.add(new Range(start, end));
         }
      }
      
      // Content features header
      String ch = request.getHeader("GetContentFeatures.DLNA.ORG");
      if ((ch != null) && ch.equals("1"))
      {
         response.setHeader("ContentFeatures.DLNA.ORG", "DLNA.ORG_PN=MTG_MPEG4P2;DLNA.ORG_OP=01;DLNA.ORG_CI=0");
         response.setHeader("TransferMode.DLNA.ORG", "Streaming");
      }

		// Send the file in the response
      response.setHeader("Accept-Ranges", "bytes");		
      response.setHeader("Content-Disposition", "attachment; filename=\"" + vf.getName() + "\"");      
		RandomAccessFile in = new RandomAccessFile(vf, "r");
		try
		{
		   ServletOutputStream out = response.getOutputStream();
			if (ranges.isEmpty())
			{
		      response.setContentType("video/mp4");
		      response.setHeader("Content-Range", "bytes 0-" + (contentLength - 1) + "/" + contentLength);
            response.setHeader("Content-Length", Integer.toString(contentLength));
            copyBytes(in, out, contentLength);
			}
			else if (ranges.size() == 1)
			{
            response.setStatus(HttpServletResponse.SC_PARTIAL_CONTENT);
            response.setContentType("video/mp4");
            Range r = ranges.get(0);
            response.setHeader("Content-Range", "bytes " + r.start + "-" + r.end + "/" + contentLength);
            response.setHeader("Content-Length", Integer.toString(r.length));
			   in.seek(r.start);
			   copyBytes(in, out, r.length);
			}
			else
			{
            response.setStatus(HttpServletResponse.SC_PARTIAL_CONTENT);
			   response.setContentType("multipart/byteranges; boundary=" + MULTIPART_BOUNDARY);
			   
			   for(Range r : ranges)
			   {
			      out.println();
			      out.println("--" + MULTIPART_BOUNDARY);
               out.println("Content-Type: video/mp4");
               out.println("Content-Range: bytes " + r.start + "-" + r.end + "/" + contentLength);
               in.seek(r.start);
               copyBytes(in, out, r.length);
			   }

			   out.println();
            out.println("--" + MULTIPART_BOUNDARY + "--");
			}
		}
		finally
		{
			in.close();
		}
	}
	
	/**
	 * @param in
	 * @param out
	 * @param total
	 * @throws IOException
	 */
	private static void copyBytes(RandomAccessFile in, OutputStream out, int total) throws IOException
	{
      byte[] buffer = new byte[Math.min(total, 8192)];
      int len = in.read(buffer);
      int bytes = total;
      while(len != -1)
      {
         out.write(buffer, 0, Math.min(len, bytes));
         bytes -= len;
         if (bytes <= 0)
            break;
         len = in.read(buffer);
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
	 * @param name
	 * @param localFile
	 */
	public static void addVideoFile(String id, File localFile)
	{
		synchronized(videoFiles)
		{
		   File vf = videoFiles.get(id);
		   if (vf != null)
		   {
		      vf.delete();
		   }
			videoFiles.put(id, localFile);
		}
	}

	/**
	 * Content range representation
	 */
	private class Range
	{
	   long start;
	   long end;
	   int length;

	   Range(long start, long end)
	   {
	      this.start = start;
	      this.end = end;
	      length = (int)(end - start + 1);
	   }
	}
}
