CREATE TABLE REPORTING_RESULTS
(
  ID decimal(10) PRIMARY KEY NOT NULL,
  EXECUTIONTIME datetime,
  REPORTID varbinary(255),
  JOBID varbinary(255),
  USERID decimal(10)
);

CREATE TABLE REPORT_NOTIFICATION
(
  ID decimal(10) PRIMARY KEY not null,
  jobid varbinary(255) not null,
  mail varchar(255) not null,
  report_name varchar(255),
  attach_report integer default 0 not null
);

CREATE SEQUENCE HIBERNATE_SEQUENCE
  INCREMENT BY 1
  MINVALUE 1
  CACHE 20;
