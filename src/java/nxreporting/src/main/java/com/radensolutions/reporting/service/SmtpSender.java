package com.radensolutions.reporting.service;

public interface SmtpSender {

    void mail(String to, String subject, String body, String fileName, byte[] fileContent);

}
