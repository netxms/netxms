package com.radensolutions.reporting.application;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Properties;

import javax.activation.DataHandler;
import javax.activation.DataSource;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.mail.Multipart;
import javax.mail.Session;
import javax.mail.Transport;
import javax.mail.internet.InternetAddress;
import javax.mail.internet.MimeBodyPart;
import javax.mail.internet.MimeMessage;
import javax.mail.internet.MimeMultipart;
import javax.mail.util.ByteArrayDataSource;

import org.netxms.api.client.reporting.ReportRenderFormat;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SMTP
{
	private static final Logger logger = LoggerFactory.getLogger(SMTP.class);
	
	private String hostname = "localhost";
	private String port = "25";
	private String from = "netxms@localhost";

	public void send(String subject, String text, String toEmail, byte[] bytes, ReportRenderFormat formatCode)
	{
		Properties properties = new Properties();
		properties.put("mail.smtp.host", hostname);
		properties.put("mail.smtp.port", port);
		Session mailSession = Session.getDefaultInstance(properties);
		
		try
		{
			Message message = new MimeMessage(mailSession);
			message.setFrom(new InternetAddress(from));
			message.setRecipients(Message.RecipientType.TO, InternetAddress.parse(toEmail));
			message.setSubject(subject);
			message.setSentDate(new Date());
			
			MimeBodyPart messagePart = new MimeBodyPart();
            messagePart.setText(text);           
            Multipart multipart = new MimeMultipart();
            multipart.addBodyPart(messagePart);
            if (bytes != null && formatCode != null)
            {
                MimeBodyPart attachmentPart = new MimeBodyPart();
                DataSource dataSource = new ByteArrayDataSource(bytes, "application/octet-stream");
                attachmentPart.setDataHandler(new DataHandler(dataSource));

                DateFormat df = new SimpleDateFormat("ddMMyyyy");
        		Date today = Calendar.getInstance().getTime();        
        		String reportDate = df.format(today);
                attachmentPart.setFileName(String.format("report_%s.%s", reportDate, formatCode.getExtension()));
                multipart.addBodyPart(attachmentPart);

            }
            message.setContent(multipart);
       
			Transport.send(message);
		} 
		catch (MessagingException e)
		{
			logger.error("Send failed, exception: " + e);
		}
	}

	public void setHostname(String hostname)
	{
		this.hostname = hostname;
	}

	public void setPort(String port)
	{
		this.port = port;
	}

	public void setFrom(String from)
	{
		this.from = from;
	}
	
}
