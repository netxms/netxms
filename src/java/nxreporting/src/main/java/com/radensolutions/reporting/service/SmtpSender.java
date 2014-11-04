package com.radensolutions.reporting.service;

import java.io.File;

public interface SmtpSender {

    void mail(String to, String subject, String body, String fileName, File file);

}
