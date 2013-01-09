
BEGIN TRY 
    BEGIN TRANSACTION; 
    USE netxms_db;
    /*Create new table with identical structure but option on*/
    SET ANSI_NULLS ON; 
    SET ANSI_PADDING OFF; 

	CREATE TABLE netxms.event_log_new
	(
	 event_id bigint not null,
	 event_code integer not null,
	 event_timestamp integer not null,
	 event_source integer not null,
	 event_severity integer not null,
	 event_message varchar(255) not null,
	 root_event_id bigint not null,
	 user_tag varchar(63) not null,
	 PRIMARY KEY(event_id)
	) ;

	CREATE INDEX idx_event_log_event_timestamp ON netxms.event_log_new(event_timestamp);


    /*Metadata only switch*/
    ALTER TABLE netxms.event_log  SWITCH TO netxms.event_log_new;

    DROP TABLE netxms.event_log; 

    EXECUTE sp_rename N'netxms.event_log_new', N'event_log','OBJECT'; 

    COMMIT TRANSACTION; 
END TRY 

BEGIN CATCH 
    IF XACT_STATE() <> 0 
      ROLLBACK TRANSACTION; 

    PRINT ERROR_MESSAGE(); 
END CATCH; 