#include <nms_common.h>
#include <nms_util.h>
#include <nxdbapi.h>
#include <testtools.h>

void TestOracleBatch();

/**
 * main()
 */
int main(int argc, char *argv[])
{
   DBInit(0, 0);

   TestOracleBatch();
   return 0;
}
