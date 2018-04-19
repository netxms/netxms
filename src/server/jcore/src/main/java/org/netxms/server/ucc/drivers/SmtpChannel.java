/**
 * 
 */
package org.netxms.server.ucc.drivers;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import org.apache.commons.mail.Email;
import org.apache.commons.mail.SimpleEmail;
import org.netxms.bridge.Platform;
import org.netxms.server.ucc.UserCommunicationChannel;

/**
 * Email channel
 */
public class SmtpChannel extends UserCommunicationChannel
{
   private static final String DEBUG_TAG = "ucc.channel.smtp";
   
   private ExecutorService executor;
   
   /**
    * @param id
    */
   public SmtpChannel(String id)
   {
      super(id);
   }

   /* (non-Javadoc)
    * @see org.netxms.server.ucc.UserCommunicationChannel#initialize()
    */
   @Override
   public void initialize() throws Exception
   {
      super.initialize();
      executor = new ThreadPoolExecutor(0, 10, 60, TimeUnit.SECONDS, new LinkedBlockingQueue<Runnable>());
   }

   /* (non-Javadoc)
    * @see org.netxms.server.ucc.UserCommunicationChannel#getDriverName()
    */
   @Override
   public String getDriverName()
   {
      return "SMTP";
   }

   /* (non-Javadoc)
    * @see org.netxms.server.ucc.UserCommunicationChannel#send(java.lang.String, java.lang.String, java.lang.String)
    */
   @Override
   public void send(String recipient, String subject, String text) throws Exception
   {
      executor.execute(new Sender(recipient, subject, text));
   }
   
   /**
    * Mail sender
    */
   private class Sender implements Runnable
   {
      private String recipient;
      private String subject;
      private String text;

      /**
       * @param recipient
       * @param subject
       * @param text
       */
      public Sender(String recipient, String subject, String text)
      {
         this.recipient = recipient;
         this.subject = subject;
         this.text = text;
      }

      /* (non-Javadoc)
       * @see java.lang.Runnable#run()
       */
      @Override
      public void run()
      {
         try
         {
            Email email = new SimpleEmail();
            email.setHostName(readConfigurationAsString("SMTPServer", "127.0.0.1"));
            email.setSmtpPort(readConfigurationAsInteger("SMTPPort", 25));
            email.setSSLOnConnect(readConfigurationAsBoolean("SSLOnConnect", false));
            email.setStartTLSEnabled(readConfigurationAsBoolean("EnableStartTLS", false));
            email.setFrom(readConfigurationAsString("FromAddress", "netxms@localhost"));
            email.setSubject(subject);
            email.setMsg(text);
            email.addTo(recipient);
            if (readConfigurationAsBoolean("Authenticate", false))
            {
               email.setAuthentication(readConfigurationAsString("Login", "netxms"), readConfigurationAsString("Password", "netxms"));
            }
            email.send();
            Platform.writeDebugLog(DEBUG_TAG, 4, "Email to " + recipient + " sent successfully");
         }
         catch(Exception e)
         {
            Platform.writeDebugLog(DEBUG_TAG, 4, "Exception in SMTP channel " + getId() + " (" + e.getClass().getName() + ")");
            Platform.writeDebugLog(DEBUG_TAG, 4, "   ", e);
         }
      }
   }
}
