package com.radensolutions.reporting.infrastructure;

public interface SmtpSender {

    void mail(String to, String subject, String body);

    void mail(String to, String subject, String body, String fileName, byte[] fileContent);

}
