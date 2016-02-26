
    -- drop table report_notifications;

    -- drop table report_results;

    create table report_notifications (
        id integer not null,
        attach_format_code integer,
        jobid varchar(255),
        mail varchar(255),
        report_name varchar(255),
        primary key (id)
    );

    create table report_results (
        id integer not null,
        executionTime datetime year to fraction(5),
        jobId varchar(255),
        reportId varchar(255),
        userId integer,
        primary key (id)
    );
