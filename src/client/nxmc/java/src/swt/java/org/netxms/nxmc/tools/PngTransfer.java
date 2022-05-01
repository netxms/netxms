/**
 * 
 */
package org.netxms.nxmc.tools;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.UncheckedIOException;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.ByteArrayTransfer;
import org.eclipse.swt.dnd.TransferData;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;

/**
 * Custom clipboard transfer to work around SWT bug 283960 that make copy image to clipboard not working on Linux 64.<br>
 * Link to SWT bug: <a>https://bugs.eclipse.org/bugs/show_bug.cgi?id=283960</a>
 */
public class PngTransfer extends ByteArrayTransfer
{
   private static final String IMAGE_PNG = "image/png";
   private static final int ID = registerType(IMAGE_PNG);

   private static PngTransfer instance = new PngTransfer();

   public static PngTransfer getInstance()
   {
      return instance;
   }

   private PngTransfer()
   {
   }

   /**
    * @see org.eclipse.swt.dnd.Transfer#getTypeNames()
    */
   @Override
   protected String[] getTypeNames()
   {
      return new String[] { IMAGE_PNG };
   }

   /**
    * @see org.eclipse.swt.dnd.Transfer#getTypeIds()
    */
   @Override
   protected int[] getTypeIds()
   {
      return new int[] { ID };
   }

   /**
    * @see org.eclipse.swt.dnd.ByteArrayTransfer#javaToNative(java.lang.Object, org.eclipse.swt.dnd.TransferData)
    */
   @Override
   protected void javaToNative(Object object, TransferData transferData)
   {
      if (object == null || !(object instanceof ImageData))
      {
         return;
      }

      if (isSupportedType(transferData))
      {
         ImageData image = (ImageData)object;
         try (ByteArrayOutputStream out = new ByteArrayOutputStream();)
         {
            // write data to a byte array and then ask super to convert to pMedium

            ImageLoader imgLoader = new ImageLoader();
            imgLoader.data = new ImageData[] { image };
            imgLoader.save(out, SWT.IMAGE_PNG);

            byte[] buffer = out.toByteArray();
            out.close();

            super.javaToNative(buffer, transferData);
         }
         catch(IOException e)
         {
            throw new UncheckedIOException(e);
         }
      }
   }

   /**
    * @see org.eclipse.swt.dnd.ByteArrayTransfer#nativeToJava(org.eclipse.swt.dnd.TransferData)
    */
   @Override
   protected Object nativeToJava(TransferData transferData)
   {
      if (isSupportedType(transferData))
      {
         byte[] buffer = (byte[])super.nativeToJava(transferData);
         if (buffer == null)
         {
            return null;
         }

         try (ByteArrayInputStream in = new ByteArrayInputStream(buffer))
         {
            return new ImageData(in);
         }
         catch(IOException e)
         {
            throw new UncheckedIOException(e);
         }
      }

      return null;
   }
}
