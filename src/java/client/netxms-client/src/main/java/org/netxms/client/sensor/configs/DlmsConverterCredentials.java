package org.netxms.client.sensor.configs;

import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

@Root(name="cred")
public class DlmsConverterCredentials
{
   public static final int RS485 = 0;
   public static final int IP485 = 1;
   public static final int RS232 = 2;
   public static final int RAW_TCP = 3;
   
   @Element(required=true)
   public int lineType;
   
   @Element(required=true)
   public int port;
   
   @Element(required=true)
   public String password;
   
   @Element(required=true)
   public String inetAddress;
   
   @Element(required=true)
   public int linkNumber;
   
   @Element(required=true)
   public int lineNumber;
   
   @Element(required=true)
   public int linkParams;
   
   
   public DlmsConverterCredentials(int lineType, int port, String password, String inetAddress)
   {
      this.inetAddress = inetAddress;
      this.lineType = lineType;
      this.port = port;
      this.password = password;
   }
   

   public DlmsConverterCredentials()
   {
      inetAddress = "";
      lineType = RAW_TCP;
      port = 0;
      password = "ABCDEFGH";
   }
}
