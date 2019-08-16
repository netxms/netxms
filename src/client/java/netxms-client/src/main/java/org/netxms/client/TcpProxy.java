/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.Timer;
import java.util.TimerTask;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * TCP proxy object
 */
public class TcpProxy
{
   private NXCSession session;
   private int channelId;
   private ProxyInputStream localInputStream;
   private ProxyOutputStream localOutputStream;
   private int timeThreshold = 100;
   private byte[] sendBuffer = new byte[256];
   private int pendingBytes = 0;
   private Timer sendTimer = new Timer(true);

   /**
    * TODO
    *
    * @param session   TODO
    * @param channelId TODO
    * @throws IOException TODO
    */
   protected TcpProxy(NXCSession session, int channelId) throws IOException
   {
      this.session = session;
      this.channelId = channelId;

      localInputStream = new ProxyInputStream();
      localOutputStream = new ProxyOutputStream();
   }

   /**
    * Close proxy session. Calling this method will also close both input and output streams.
    */
   public void close()
   {
      if (session == null)
         return;

      session.closeTcpProxy(channelId);
      localClose();
   }

   /**
    * Close local resources associated with proxy session.
    * Should only be called by remote proxy session closure notification handler.
    */
   protected void localClose()
   {
      sendTimer.cancel();
      session = null;
      try
      {
         localOutputStream.close();
         localInputStream.close();
      }
      catch(Exception e)
      {
      }
      localInputStream = null;
      localOutputStream = null;
   }

   /**
    * Check if proxy session is closed.
    *
    * @return true if proxy session is closed
    */
   public boolean isClosed()
   {
      return session == null;
   }

   /**
    * Get input stream for this TCP proxy.
    *
    * @return input stream for this TCP proxy
    */
   public InputStream getInputStream()
   {
      return localInputStream;
   }

   /**
    * Get output stream for this TCP proxy.
    *
    * @return output stream for this TCP proxy
    */
   public OutputStream getOutputStream()
   {
      return localOutputStream;
   }

   /**
    * Get channel ID
    *
    * @return channel ID
    */
   protected int getChannelId()
   {
      return channelId;
   }

   /**
    * Get current size threshold.
    *
    * @return current size threshold
    */
   public int getSizeThreshold()
   {
      return sendBuffer.length;
   }

   /**
    * Get current time threshold.
    *
    * @return current time threshold
    */
   public int getTimeThreshold()
   {
      return timeThreshold;
   }

   /**
    * Set write buffering thresholds. If these parameters are non-zero,
    * proxy object will buffer outgoing data until it reach size threshold,
    * but not longer that time threshold.
    *
    * @param sizeThreshold data size threshold in bytes
    * @param timeThreshold time threshold in milliseconds
    */
   public void setBufferingThresholds(int sizeThreshold, int timeThreshold)
   {
      this.sendBuffer = new byte[sizeThreshold];
      this.timeThreshold = timeThreshold;
   }

   /**
    * Send data to destination
    *
    * @param data data block
    * @throws IOException  TODO
    * @throws NXCException TODO
    */
   public synchronized void send(byte[] data) throws IOException, NXCException
   {
      if (pendingBytes + data.length < sendBuffer.length)
      {
         appendBytes(sendBuffer, pendingBytes, data, data.length);
         if (pendingBytes == 0)
         {
            sendTimer.schedule(new TimerTask()
            {
               @Override
               public void run()
               {
                  flushSendBuffer();
               }
            }, timeThreshold);
         }
         pendingBytes += data.length;
         return;
      }

      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_TCP_PROXY_DATA, channelId);
      msg.setBinaryMessage(true);
      if (pendingBytes > 0)
      {
         byte[] buffer;
         if (pendingBytes + data.length <= sendBuffer.length)
         {
            appendBytes(sendBuffer, pendingBytes, data, data.length);
            buffer = sendBuffer;
         }
         else
         {
            buffer = new byte[pendingBytes + data.length];
            appendBytes(buffer, 0, sendBuffer, pendingBytes);
            appendBytes(buffer, pendingBytes, data, data.length);
         }
         msg.setBinaryData(buffer);
         pendingBytes = 0;
      }
      else
      {
         msg.setBinaryData(data);
      }
      session.sendMessage(msg);
   }

   /**
    * Flush send buffer
    */
   private synchronized void flushSendBuffer()
   {
      if (pendingBytes == 0)
         return;

      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_TCP_PROXY_DATA, channelId);
      msg.setBinaryMessage(true);
      msg.setBinaryData((pendingBytes == sendBuffer.length) ? sendBuffer : Arrays.copyOfRange(sendBuffer, 0, pendingBytes));
      try
      {
         session.sendMessage(msg);
      }
      catch(Exception e)
      {
      }
      pendingBytes = 0;
   }

   /**
    * Process data received from remote end
    *
    * @param data data received
    */
   protected void processRemoteData(byte data[])
   {
      localInputStream.write(data);
   }

   /**
    * Append bytes from one byte array to another
    *
    * @param target target array
    * @param offset offset within target array
    * @param source source array
    * @param len length of source data
    */
   private static void appendBytes(byte[] target, int offset, byte[] source, int len)
   {
      for(int i = 0, j = offset; i < len; i++, j++)
         target[j] = source[i];
   }

   /**
    * Proxy output stream
    */
   private class ProxyOutputStream extends OutputStream
   {
      /**
       * @see java.io.OutputStream#write(int)
       */
      @Override
      public void write(int b) throws IOException
      {
         byte[] data = new byte[1];
         data[0] = (byte)b;
         write(data, 0, 1);
      }

      /**
       * @see java.io.OutputStream#write(byte[], int, int)
       */
      @Override
      public void write(byte[] b, int off, int len) throws IOException
      {
         try
         {
            if ((off == 0) && (len == b.length))
            {
               send(b);
            }
            else
            {
               send(Arrays.copyOfRange(b, off, len));
            }
         }
         catch(NXCException e)
         {
            throw new IOException(e);
         }
      }
   }
   
   /**
    * Proxy input stream
    */
   private class ProxyInputStream extends InputStream
   {
      private boolean closed = false;
      private byte[] buffer = new byte[65536];
      private int readPos = 0;
      private int writePos = 0;
      private Object monitor = new Object();
      
      /**
       * Write bytes to stream internal buffer
       * 
       * @param data data to write
       */
      public void write(byte[] data)
      {
         synchronized(monitor)
         {
            if (buffer.length - writePos < data.length)
            {
               if (buffer.length - writePos + readPos >= data.length)
               {
                  // Buffer can be shifted to make place for new data
                  System.arraycopy(buffer, readPos, buffer, 0, writePos - readPos);
               }
               else
               {
                  byte[] newBuffer = new byte[Math.max(buffer.length * 2, data.length + buffer.length)];
                  System.arraycopy(buffer, readPos, newBuffer, 0, writePos - readPos);
                  buffer = newBuffer;
               }
               writePos -= readPos; 
               readPos = 0;
            }
            System.arraycopy(data, 0, buffer, writePos, data.length);
            writePos += data.length;
         }
      }
      
      /**
       * @see java.io.InputStream#close()
       */
      @Override
      public void close() throws IOException
      {
         synchronized(monitor)
         {
            closed = true;
            monitor.notify();
         }
      }

      /**
       * @see java.io.InputStream#available()
       */
      @Override
      public int available() throws IOException
      {
         synchronized(monitor)
         {
            return writePos - readPos;
         }
      }

      /**
       * @see java.io.InputStream#read()
       */
      @Override
      public int read() throws IOException
      {
         synchronized(monitor)
         {
            while(readPos == writePos)
            {
               if (closed)
                  return -1;
               try
               {
                  monitor.wait();
               }
               catch(InterruptedException e)
               {
               }
            }
            int b = buffer[readPos++];
            if (readPos == writePos)
            {
               readPos = 0;
               writePos = 0;
            }
            return b < 0 ? 129 + b : b;
         }
      }

      /**
       * @see java.io.InputStream#read(byte[], int, int)
       */
      @Override
      public int read(byte[] b, int off, int len) throws IOException
      {
         if (len == 0)
            return 0;

         synchronized(monitor)
         {
            while(readPos == writePos)
            {
               if (closed)
                  return -1;
               try
               {
                  monitor.wait();
               }
               catch(InterruptedException e)
               {
               }
            }
            
            int bytes = Math.min(len, writePos - readPos);
            System.arraycopy(buffer, readPos, b, off, bytes);
            readPos += bytes;
            if (readPos == writePos)
            {
               readPos = 0;
               writePos = 0;
            }
            return bytes;
         }
      }

      /**
       * @see java.io.InputStream#read(byte[])
       */
      @Override
      public int read(byte[] b) throws IOException
      {
         return read(b, 0, b.length);
      }
   }
}
