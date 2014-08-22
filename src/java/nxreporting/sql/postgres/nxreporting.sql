CREATE TABLE reporting_results (
    id integer NOT NULL,
    executiontime timestamp without time zone,
    jobid bytea,
    reportid bytea,
    userid integer
);

ALTER TABLE ONLY reporting_results
    ADD CONSTRAINT reporting_results_pkey PRIMARY KEY (id);

CREATE TABLE report_notification (
    id integer NOT NULL,
    attach_format_code integer,
    jobid bytea,
    mail character varying(255),
    report_name character varying(255)
);

ALTER TABLE ONLY report_notification
    ADD CONSTRAINT report_notification_pkey PRIMARY KEY (id);

CREATE SEQUENCE HIBERNATE_SEQUENCE INCREMENT BY 1 MINVALUE 1;
