package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import java.io.ByteArrayInputStream;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;
import org.apache.commons.codec.binary.Base64;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementList;

public abstract class GenericMenuItem
{
   protected static final int ITEM = 1;
   protected static final int FOLDER = 2;
   
   
   @Element(required=false)
   public String name;
   
   @Element(required=false)
   public String discription;
   
   @Element(required=false)
   public String command;
   
   @ElementList(required = true)
   public List<GenericMenuItem> children = new ArrayList<GenericMenuItem>();  
   
   @Element(required=false)
   public String icon;
   
   @Element(required=false)
   public int type;

   public GenericMenuItem parent; 

   
   public abstract String getCommand();
   public abstract boolean hasChildren();
   public abstract List<GenericMenuItem> getChildren();
   public abstract GenericMenuItem getParent();

   public void setIcon(byte[] bs)
   {
      try
      {
         icon = new String(Base64.encodeBase64(bs, false), "ISO-8859-1");
      }
      catch(UnsupportedEncodingException e)
      {
         icon = "";
      }
   }
   
   public void updateParents(GenericMenuItem parent) 
   {
   	this.parent = parent;
   	for(GenericMenuItem child : children)
   	   child.updateParents(this);
   }
   
   public boolean delete()
   {
      if (parent != null)
      {
         parent.removeChild(this);
         clearChildren();
         return true;
      }
      return false;
   }
   
   protected void clearChildren()
   {
      for(GenericMenuItem obj : children)
      {
         obj.clearChildren();
      }
      children = null;
   }

   protected void removeChild(GenericMenuItem item)
   {
      children.remove(item);
   }
   
   public void addChild(GenericMenuItem item)
   {
      children.add(item);
   }
   
   public Image getIcon()
   {
      if(icon == null)
         return null;
      try
      {
         byte[] tmp = Base64.decodeBase64(icon.getBytes("ISO-8859-1"));
         ByteArrayInputStream input = new ByteArrayInputStream(tmp);
         ImageDescriptor d = ImageDescriptor.createFromImageData(new ImageData(input));
         return d.createImage();
      }
      catch(Exception e)
      {
         return null;
      }
   }
   

   public String getName()
   {
      return name;
   }
   
   public String getDiscriptionName()
   {
      return discription;
   }
}
