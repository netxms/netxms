
    -- drop table report_notifications cascade constraints;

    -- drop table report_results cascade constraints;

    create table report_notifications (
        id number(10,0) not null,
        attach_format_code number(10,0),
        jobid varchar2(255 char),
        mail varchar2(255 char),
        report_name varchar2(255 char),
        primary key (id)
    );

    create table report_results (
        id number(10,0) not null,
        executionTime timestamp,
        jobId varchar2(255 char),
        reportId varchar2(255 char),
        userId number(10,0),
        primary key (id)
    );
