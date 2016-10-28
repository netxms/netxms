CREATE TABLE REPORTING_RESULTS
(
  ID decimal(10) PRIMARY KEY NOT NULL,
  EXECUTIONTIME timestamp,
  REPORTID char(36),
  JOBID char(36),
  USERID decimal(10)
);

CREATE TABLE REPORT_NOTIFICATION
(
  ID decimal(10) PRIMARY KEY not null,
  jobid char(36) not null,
  mail varchar(255) not null,
  report_name varchar(255),
  attach_format_code integer default 0 not null
);

CREATE SEQUENCE HIBERNATE_SEQUENCE
  INCREMENT BY 1
  MINVALUE 1
  MAXVALUE 9999999999999999999999999999
  CACHE 20
  NOCYCLE;
