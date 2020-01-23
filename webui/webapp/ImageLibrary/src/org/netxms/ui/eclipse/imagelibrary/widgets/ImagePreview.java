/**
 * 
 */
package org.netxms.ui.eclipse.imagelibrary.widgets;

import java.io.ByteArrayInputStream;
import org.eclipse.swt.SWT;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.LibraryImage;
import org.netxms.ui.eclipse.imagelibrary.Activator;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;
import org.netxms.ui.eclipse.tools.FontTools;

/**
 * Image preview widget for image library
 */
public class ImagePreview extends Composite
{
   private static final String[] HEADER_FONTS = { "Verdana", "DejaVu Sans", "Liberation Sans", "Arial" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$

   private LibraryImage imageDescriptor;
   private Image image;
   private Label imageName;
   private Label imageSize;
   private Canvas imagePreview;
   private Font headerFont;

   /**
    * @param parent
    * @param style
    */
   public ImagePreview(Composite parent, int style)
   {
      super(parent, style);
   
      headerFont = FontTools.createFont(HEADER_FONTS, +3, SWT.BOLD);
      
      setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
      
      GridLayout layout = new GridLayout();
      setLayout(layout);
      
      imageName = new Label(this, SWT.NONE);
      imageName.setBackground(getBackground());
      imageName.setText("No image selected");
      imageName.setFont(headerFont);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      imageName.setLayoutData(gd);

      imageSize = new Label(this, SWT.NONE);
      imageSize.setBackground(getBackground());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      imageSize.setLayoutData(gd);
      
      imagePreview = new Canvas(this, SWT.DOUBLE_BUFFERED);
      imagePreview.setBackground(getBackground());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      imagePreview.setLayoutData(gd);
      imagePreview.addPaintListener(new PaintListener() {
         @Override
         public void paintControl(PaintEvent e)
         {
            drawImagePreview(e.gc);
         }
      });
      
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            headerFont.dispose();
            if (image != null)
               image.dispose();
         }
      });
   }
   
   /**
    * Set new image to display
    * 
    * @param imageDescriptor new image to display
    */
   public void setImage(LibraryImage imageDescriptor)
   {
      if ((imageDescriptor == null) && (this.imageDescriptor == null))
         return;
      
      this.imageDescriptor = imageDescriptor;
      imageName.setText((imageDescriptor != null) ? imageDescriptor.getName() : "No image selected"); 
      
      if (image != null)
         image.dispose();
      image = createImageFromDescriptor(getDisplay(), imageDescriptor);
      
      if (image != null)
      {
         Rectangle rect = image.getBounds();
         imageSize.setText(String.format("%d x %d", rect.width, rect.height));
      }
      else
      {
         imageSize.setText("");
      }
      
      imagePreview.redraw();
   }
   
   /**
    * @param gc
    */
   private void drawImagePreview(GC gc)
   {
      Rectangle rect = imagePreview.getClientArea();
      rect.width -= 2;
      rect.height -= 2;
      rect.x++;
      rect.y++;
      if (image == null)
      {
         if (rect.height > rect.width)
            rect.height = rect.width;
         else
            rect.width = rect.height;
         gc.drawRectangle(rect);
         gc.drawLine(0, 0, rect.width, rect.height);
         gc.drawLine(rect.width, 0, 0, rect.height);
      }
      else
      {
         Rectangle imageBounds = image.getBounds();
         if ((imageBounds.width > rect.width) || (imageBounds.height > rect.height))
         {
            try
            {
               gc.setAntialias(SWT.ON);
            }
            catch(SWTException e)
            {
               // Ignore SWT exception and continue with current GC settings
            }

            float scaleFactorW = (float)imageBounds.width / (float)rect.width;
            float scaleFactorH = (float)imageBounds.height / (float)rect.height;
            float scaleFactor = Math.max(scaleFactorW, scaleFactorH);
            gc.drawImage(image, 0, 0, imageBounds.width, imageBounds.height, 0, 0,
                  (int)((float)imageBounds.width / scaleFactor), (int)((float)imageBounds.height / scaleFactor));
         }
         else
         {
            gc.drawImage(image, 0, 0);
         }
      }
   }
   
   /**
    * Create image from library descriptor
    * 
    * @param display
    * @param imageDescriptor
    * @return
    */
   private static Image createImageFromDescriptor(Display display, LibraryImage imageDescriptor)
   {
      if (imageDescriptor == null)
         return null;
      
      if (imageDescriptor.getBinaryData() != null)
      {
         try
         {
            return new Image(display, new ByteArrayInputStream(imageDescriptor.getBinaryData()));
         }
         catch(Exception e)
         {
            Activator.logError("Cannot create image from library descriptor", e);
            return null;
         }
      }
      
      Image image = ImageProvider.getInstance().getImage(imageDescriptor.getGuid());
      return (image != null) ? new Image(display, image, SWT.IMAGE_COPY) : null;
   }
}
