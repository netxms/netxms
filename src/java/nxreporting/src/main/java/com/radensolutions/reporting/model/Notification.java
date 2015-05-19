package com.radensolutions.reporting.model;

import org.hibernate.annotations.Type;

import javax.persistence.*;
import java.io.Serializable;
import java.util.UUID;

@Entity
@Table(name = "report_notification")
public class Notification implements Serializable {
    @Id
    @Column(name = "id")
    @GeneratedValue
    private Integer id;

    @Column(name = "jobid")
    @Type(type="uuid-char")
    private UUID jobId;

    @Column(name = "mail")
    private String mail;

    @Column(name = "attach_format_code")
    private int attachFormatCode;

    @Column(name = "report_name")
    private String reportName;

    public Notification() {
    }

    public Notification(UUID jobId, String mail, int attachFormatCode, String reportName) {
        this.jobId = jobId;
        this.mail = mail;
        this.attachFormatCode = attachFormatCode;
        this.reportName = reportName;
    }

    public Integer getId() {
        return id;
    }

    public void setId(Integer id) {
        this.id = id;
    }

    public UUID getJobId() {
        return jobId;
    }

    public void setJobId(UUID jobId) {
        this.jobId = jobId;
    }

    public String getMail() {
        return mail;
    }

    public void setMail(String mail) {
        this.mail = mail;
    }

    public int getAttachFormatCode() {
        return attachFormatCode;
    }

    public void setAttachFormatCode(int attachFormatCode) {
        this.attachFormatCode = attachFormatCode;
    }

    public String getReportName() {
        return reportName;
    }

    public void setReportName(String reportName) {
        this.reportName = reportName;
    }
}
