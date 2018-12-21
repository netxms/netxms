package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import java.io.UnsupportedEncodingException;
import org.apache.commons.codec.binary.Base64;
import org.eclipse.swt.graphics.Image;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementArray;
import org.simpleframework.xml.Root;

@Root(name="item")
public abstract class GenericMenuItem
{
   protected static final int ITEM = 1;
   protected static final int FOLDER = 2;
   
   
   @Element(required=false)
   public String name;
   
   @Element(required=false)
   public String displayName;
   
   @Element(required=false)
   public String command;
   
   @ElementArray(required = true)
   public GenericMenuItem children[] = new GenericMenuItem[0];   
   
   @Element(required=false)
   public String icon;
   
   @Element(required=false)
   public int type;

   public GenericMenuItem parent; 

   
   public abstract String getName();
   public abstract String getDisplayName();
   public abstract String getCommand();
   public abstract Image getIcon();
   public abstract boolean hasChildren();
   public abstract GenericMenuItem[] getChildren();
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
}
