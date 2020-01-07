package org.netxms.client.objects.configs;

import java.io.StringWriter;
import java.io.Writer;
import java.util.UUID;
import org.netxms.base.NXCommon;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Information about object placement  
 */
@Root(name="placement")
public class ChassisPlacement
{
   static final double UNIT_HEIGHT = 44.45;
   static final double HORIZONTAL_PITCH = 5.08;
   
   @Element(required=false)
   private String image;
   
   @Element(required=false)
   private int height;
   
   @Element(required=false)
   private int heightUnits;

   @Element(required=false)
   private int width;

   @Element(required=false)
   private int widthUnits;

   @Element(required=false)
   private int positionHeight;

   @Element(required=false)
   private int positionHeightUnits;

   @Element(required=false)
   private int positionWidth;

   @Element(required=false)
   private int positionWidthUnits;

   @Element(required=false)
   private int oritentaiton;
   

   /**
    * Create DCI list object from XML document
    * 
    * @param xml XML document
    * @return deserialized object
    * @throws Exception if the object cannot be fully deserialized
    */
   public static ChassisPlacement createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      return serializer.read(ChassisPlacement.class, xml);
   }
   
   /**
    * Create XML from configuration.
    * 
    * @return XML document
    * @throws Exception if the schema for the object is not valid
    */
   public String createXml() throws Exception
   {
      Serializer serializer = new Persister();
      Writer writer = new StringWriter();
      serializer.write(this, writer);
      return writer.toString();
   }
   
   /**
    * Default constructor
    */
   public ChassisPlacement()
   {
      image = NXCommon.EMPTY_GUID.toString();
      height = 0;
      heightUnits = 0;
      width = 0;
      widthUnits = 0;
      positionHeight = 0;
      positionHeightUnits = 0;
      positionWidth = 0;
      positionWidthUnits = 0;
      oritentaiton = 1;
   }
   
   /**
    * Constructor
    */
   public ChassisPlacement(UUID image, int height, int heightUnits, int width, int widthUnits, int positionHeight, 
         int positionHeightUnits, int positionWidth, int positionWidthUnits, int oritentaiton)
   {
      this.image = image.toString();
      this.height = height;
      this.heightUnits = heightUnits;
      this.width = width;
      this.widthUnits = widthUnits;
      this.positionHeight = positionHeight;
      this.positionHeightUnits = positionHeightUnits;
      this.positionWidth = positionWidth;
      this.positionWidthUnits = positionWidthUnits;
      this.oritentaiton = oritentaiton;
   }
   
   /**
    * Returns image that will be used to display this element on chassis
    * 
    * @return image UUID
    */
   public UUID getImage()
   {
      return UUID.fromString(image);
   }

   /**
    * @return the height
    */
   public int getHeight()
   {
      return height;
   }

   /**
    * @return the heightUnits
    */
   public int getHeightUnits()
   {
      return heightUnits;
   }

   /**
    * @return the width
    */
   public int getWidth()
   {
      return width;
   }

   /**
    * @return the widthUnits
    */
   public int getWidthUnits()
   {
      return widthUnits;
   }

   /**
    * @return the positionHeight
    */
   public int getPositionHeight()
   {
      return positionHeight;
   }

   /**
    * @return the positionHeightUnits
    */
   public int getPositionHeightUnits()
   {
      return positionHeightUnits;
   }

   /**
    * @return the positionWidth
    */
   public int getPositionWidth()
   {
      return positionWidth;
   }

   /**
    * @return the positionWidthUnits
    */
   public int getPositionWidthUnits()
   {
      return positionWidthUnits;
   }

   /**
    * @return the oritentaiton
    */
   public int getOritentaiton()
   {
      return oritentaiton;
   }
   
   /**
    * Get unit position height in mm
    * 
    * @return position height in mm
    */
   public double getPositionHeightInMm()
   {
      if (positionHeightUnits == 0)
      {
         return UNIT_HEIGHT * positionHeight;
      }
      return positionHeight;
   }
   
   /**
    * Get unit position width in mm
    * 
    * @return position width in mm
    */
   public double getPositionWidthInMm()
   {
      if (positionWidthUnits == 0)
      {
         return HORIZONTAL_PITCH * positionWidth;
      }
      return positionWidth;      
   }

   /**
    * Get unit height in mm
    * 
    * @return unit height in mm
    */
   public double getHeightInMm()
   {
      if (heightUnits == 0)
      {
         return UNIT_HEIGHT * height;
      }
      return height;
   }
   
   /**
    * Get unit unit in mm
    * 
    * @return unit height in mm
    */
   public double getWidthInMm()
   {
      if (widthUnits == 0)
      {
         return HORIZONTAL_PITCH * width;
      }
      return width;
   }
}
