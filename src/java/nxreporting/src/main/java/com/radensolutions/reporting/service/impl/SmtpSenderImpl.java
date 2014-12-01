package com.radensolutions.reporting.service.impl;

import java.io.File;
import javax.mail.MessagingException;
import javax.mail.internet.MimeMessage;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.mail.javamail.JavaMailSender;
import org.springframework.mail.javamail.MimeMessageHelper;
import org.springframework.stereotype.Component;
import com.radensolutions.reporting.service.SmtpSender;

@Component
public class SmtpSenderImpl implements SmtpSender {

    Logger log = LoggerFactory.getLogger(SmtpSenderImpl.class);

    @Autowired
    private JavaMailSender mailSender;

    @Value("#{serverSettings.smtpFrom}")
    private String from;

    @Override
    public void mail(String to, String subject, String body, String fileName, File file) {
        try {
            MimeMessage message = mailSender.createMimeMessage();
            MimeMessageHelper helper = new MimeMessageHelper(message, true);
            helper.setFrom(from);
            helper.setTo(to);
            helper.setSubject(subject);
            if (fileName != null && file != null) {
                helper.addAttachment(fileName, file);
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
