
    -- drop table if exists report_notifications cascade;

    -- drop table if exists report_results cascade;

    create table report_notifications (
        id int4 not null,
        attach_format_code int4,
        jobid varchar(255),
        mail varchar(255),
        report_name varchar(255),
        primary key (id)
    );

    create table report_results (
        id int4 not null,
        executionTime timestamp,
        jobId varchar(255),
        reportId varchar(255),
        userId int4,
        primary key (id)
    );
