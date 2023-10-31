/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * TCP proxy object
 */
public class TcpProxy
{
   private static final Logger logger = LoggerFactory.getLogger(TcpProxy.class);

   private NXCSession session;
   private int channelId;
   private ProxyInputStream localInputStream;
   private ProxyOutputStream localOutputStream;
   private int timeThreshold = 100;
   private byte[] sendBuffer = new byte[256];
   private int pendingBytes = 0;
   private Timer sendTimer = new Timer(true);
   private Exception flushException = null;

   /**
    * Create new TCP proxy object.
    *
    * @param session underlying NetXMS client session
    * @param channelId proxy channel ID
    * @throws IOException when input or output stream cannot be created
    */
   protected TcpProxy(NXCSession session, int channelId) throws IOException
   {
      this.session = session;
      this.channelId = channelId;

      localInputStream = new ProxyInputStream();
      localOutputStream = new ProxyOutputStream();

      logger.debug("New TCP proxy object created for channel " + channelId);
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
   protected synchronized void localClose()
   {
      logger.debug("Local close for TCP proxy channel " + channelId);
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
    * Abort TCP proxy session due to external error. Next attempt to read from this proxy input stream will throw IOException.
    *
    * @param cause cause for abort
    */
   protected synchronized void abort(Throwable cause)
   {
      logger.debug("Abort for TCP proxy channel " + channelId, cause);
      localInputStream.setException(cause);
      sendTimer.cancel();
      session = null;
      try
      {
         localOutputStream.close();
      }
      catch(Exception e)
      {
      }
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
    * Get last exception generated when flushing output buffer.
    *
    * @return last exception generated when flushing output buffer or null
    */
   public Exception getFlushException()
   {
      return flushException;
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
    * @throws IOException when client cannot send data to the server or channel is already closed
    * @throws NXCException when NetXMS server cannot accept or forward data
    */
   public synchronized void send(byte[] data) throws IOException, NXCException
   {
      if (flushException != null)
         throw new IOException(flushException);

      if (isClosed())
         throw new IOException("Proxy channel is closed");

      if (pendingBytes + data.length < sendBuffer.length)
      {
         appendBytes(sendBuffer, pendingBytes, data, data.length);
         if (pendingBytes == 0)
         {
            sendTimer.schedule(new TimerTask() {
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
      if ((pendingBytes == 0) || isClosed())
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
         logger.warn("Error flushing TCP proxy buffer", e);
         flushException = e; // Next attempt to send data will abort
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
    * Proxy output stream. Writing to this stream will send data to remote system.
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
    * Proxy input stream. Reading from this stream will retrieve data received from remote system.
    */
   private class ProxyInputStream extends InputStream
   {
      private boolean closed = false;
      private byte[] buffer = new byte[65536];
      private int readPos = 0;
      private int writePos = 0;
      private Object monitor = new Object();
      private Throwable exception = null;

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
            monitor.notify();
         }
      }

      /**
       * Set stream to exception state. Next attempt to read from this stream will throw IOException.
       *
       * @param exception cause for IOException to be thrown
       */
      public void setException(Throwable exception)
      {
         synchronized(monitor)
         {
            this.exception = exception;
            monitor.notify();
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
               if (exception != null)
                  throw new IOException(exception);
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
               if (exception != null)
                  throw new IOException(exception);
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
