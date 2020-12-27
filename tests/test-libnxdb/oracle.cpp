#include <nms_common.h>
#include <nms_util.h>
#include <nxdbapi.h>
#include <testtools.h>

/**
 * Test batches in Oracle
 */
void TestOracleBatch(const TCHAR *server, const TCHAR *login, const TCHAR *password)
{
   /*** connect ***/
   StartTest(_T("Oracle: connect to database"));

   DB_DRIVER drv = DBLoadDriver(_T("oracle.ddr"), _T(""), NULL, NULL);
   AssertNotNull(drv);

   TCHAR buffer[DBDRV_MAX_ERROR_TEXT];
   DB_HANDLE session = DBConnect(drv, server, NULL, login, password, NULL, buffer);
   AssertNotNull(session);

   EndTest();

   /*** drop test table if exist ***/
   if (DBIsTableExist(session, _T("nx_test")) == DBIsTableExist_Found)
      DBQuery(session, _T("DROP TABLE nx_test"));

   /*** create test table ***/
   StartTest(_T("Oracle: create test table"));
   AssertTrueEx(DBQueryEx(session, _T("CREATE TABLE nx_test (id integer not null,value1 varchar(63), value2 integer, PRIMARY KEY(id))"), buffer), buffer);
   AssertTrue(DBIsTableExist(session, _T("nx_test")) == DBIsTableExist_Found);
   EndTest();

   /*** insert single record ***/
   StartTest(_T("Oracle: insert single record"));
   DB_STATEMENT hStmt = DBPrepareEx(session, _T("INSERT INTO nx_test (id,value1,value2) VALUES (?,?,?)"), true, buffer);
   AssertNotNullEx(hStmt, buffer);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (INT32)0);
#ifdef UNICODE
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, WideStringFromMBString(__FILE__), DB_BIND_DYNAMIC);
#else
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, __FILE__, DB_BIND_STATIC);
#endif
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)42);
   AssertTrueEx(DBExecuteEx(hStmt, buffer), buffer);
   DBFreeStatement(hStmt);
   EndTest();

   /*** check record count ***/
   StartTest(_T("Oracle: check record count"));
   DB_RESULT hResult = DBSelectEx(session, _T("SELECT count(*) FROM nx_test"), buffer);
   AssertNotNullEx(hResult, buffer);
   AssertTrue(DBGetFieldLong(hResult, 0, 0) == 1);
   DBFreeResult(hResult);
   EndTest();

   /*** do bulk insert ***/
   StartTest(_T("Oracle: bulk insert"));
   hStmt = DBPrepareEx(session, _T("INSERT INTO nx_test (id,value1,value2) VALUES (?,?,?)"), true, buffer);
   AssertNotNullEx(hStmt, buffer);
   AssertTrueEx(DBOpenBatch(hStmt), _T("Call to DBOpenBatch() failed"));
   for(INT32 i = 1; i <= 110; i++)
   {
      DBNextBatchRow(hStmt);
      if (i == 1)
      {
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)42);
      }
      TCHAR *text = (TCHAR *)calloc(256, sizeof(TCHAR));
      _sntprintf(text, 256, _T("i=%d ABCDEFGHIJKLMNO"), i);
      text[i % 8 + 6] = 0;
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, text, DB_BIND_DYNAMIC);
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, i);
   }
   AssertTrueEx(DBExecuteEx(hStmt, buffer), buffer);
   DBFreeStatement(hStmt);
   EndTest();

   /*** check record count after bulk insert ***/
   StartTest(_T("Oracle: check record count"));
   hResult = DBSelectEx(session, _T("SELECT count(*) FROM nx_test"), buffer);
   AssertNotNullEx(hResult, buffer);
   AssertTrue(DBGetFieldLong(hResult, 0, 0) == 111);
   DBFreeResult(hResult);
   EndTest();

   /*** drop test table ***/
   StartTest(_T("Oracle: drop test table"));
   AssertTrue(DBQuery(session, _T("DROP TABLE nx_test")));
   EndTest();

   /*** disconnect ***/
   StartTest(_T("Oracle: disconnect from database"));
   DBDisconnect(session);
   DBUnloadDriver(drv);
   EndTest();
}
