-- sqlite> .mode insert

-- sqlite> .schema websvc_definitions
-- CREATE TABLE websvc_definitions (    id integer not null,    guid varchar(36) not null,    name varchar(63) not null,    description varchar(2000) null,    url varchar(4000) null,    http_request_method integer not null,    request_data varchar(4000) null,    flags integer not null,    auth_type integer not null,    login varchar(255) null,    password varchar(255) null,    cache_retention_time integer not null,    request_timeout integer not null,    PRIMARY KEY(id) );
-- sqlite> select * from websvc_definitions ;
INSERT INTO "websvc_definitions" VALUES(1,'fd69a06e-082a-41e2-b4d8-e1132e23998c','nx2488webservice','','http://127.0.0.1:1500',0,'',7,0,'','',0,0);

-- sqlite> .schema script_library
-- CREATE TABLE script_library (   guid varchar(36) not null,   script_id integer not null,   script_name varchar(255) not null,   script_code varchar null,   PRIMARY KEY(script_id) );
-- sqlite> select * from script_library where script_name = 'nx2488script';
INSERT INTO "script_library" VALUES('c8336b1d-7a62-4b22-b400-2d27530bf21d',10003,'nx2488script',replace('node = FindObject("netxms"); // FIXME HARDCODE\nres1 = node.callWebService("nx2488webservice", "GET", "/");\nres2 = node.callWebService("nx2488webservice", "GET", "/");\nassert(res1.success);\nassert(res2.success);\nprintln(res1.document .. " vs " .. res2.document);\nassert(res1.document != res2.document);','\n',char(10)));

-- Drop the "must change password" flag. Let the script use the default password with nxadm.
UPDATE users SET flags = 0 WHERE name = 'admin';
