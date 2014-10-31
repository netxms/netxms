package com.radensolutions.reporting.service.impl;

import com.radensolutions.reporting.service.SmtpSender;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.core.io.ByteArrayResource;
import org.springframework.mail.javamail.JavaMailSender;
import org.springframework.mail.javamail.MimeMessageHelper;
import org.springframework.stereotype.Component;

import javax.mail.MessagingException;
import javax.mail.internet.MimeMessage;

@Component
public class SmtpSenderImpl implements SmtpSender {

    Logger log = LoggerFactory.getLogger(SmtpSenderImpl.class);

    @Autowired
    private JavaMailSender mailSender;

    @Value("#{serverSettings.smtpFrom}")
    private String from;

    @Override
    public void mail(String to, String subject, String body, String fileName, byte[] fileContent) {
        try {
            MimeMessage message = mailSender.createMimeMessage();
            MimeMessageHelper helper = new MimeMessageHelper(message, true);
            helper.setFrom(from);
            helper.setTo(to);
            helper.setSubject(subject);
            if (fileName != null && fileContent != null) {
                helper.addAttachment(fileName, new ByteArrayResource(fileContent));
            }
            helper.setText(body);
            mailSender.send(message);
        } catch (MessagingException e) {
            log.error("Unable to send notification mail", e);
        }
    }

    public void setFrom(String from) {
        this.from = from;
    }
}
