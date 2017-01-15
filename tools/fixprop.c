#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <iconv.h>

int main(int argc, char *argv[])
{
   if (argc != 3)
   {
      printf("Usage: fixprop <file> <encoding>\n");
      return 1;
   }

   iconv_t cd = iconv_open("UCS-2", argv[2]);
   if (cd == (iconv_t)-1)
   {
      printf("iconv_open failed (%s)\n", strerror(errno));      
      return 4;
   }

   FILE *in = fopen(argv[1], "r");
   if (in == NULL)
   {
      printf("Cannot open input file (%s)\n", strerror(errno));
      return 2;
   }

   printf("Processing %s ... ", argv[1]);

   char outfile[1024];
   snprintf(outfile, 1024, "%s.tmp", argv[1]);
   FILE *out = fopen(outfile, "w");
   if (out == NULL)
   {
      printf("Cannot open output file (%s)\n", strerror(errno));
      fclose(in);
      return 3;
   }

   int errors = 0;
   while(!feof(in))
   {
      char line[8192];
      if (fgets(line, 8192, in) == NULL)
         break;

      size_t inputLen = strlen(line);
      unsigned short *jtext = (unsigned short *)malloc((inputLen + 1) * 2);

      iconv(cd, NULL, NULL, NULL, NULL);

      size_t outputLen = inputLen * 2 + 2;
      char *inbuf = line;
      char *outbuf = (char *)jtext;
      size_t s = iconv(cd, &inbuf, &inputLen, &outbuf, &outputLen);
      if (s != (size_t)-1)
      {
         *outbuf = 0;
         *(outbuf + 1) = 0;
         int j = 0;
         for(int i = 0; jtext[i] != 0; i++)
         {
            if (jtext[i] < 128)
            {
               line[j++] = (char)jtext[i];
            }
            else
            {
               sprintf(&line[j], "\\u%04x", jtext[i]);
               j += 6;
            }
         }
         line[j] = 0;
         fwrite(line, strlen(line), 1, out);
      }
      else
      {
         //printf("iconv() error (%s): text=\"%s\"\n", strerror(errno), line);
         errors++;
         fwrite(line, strlen(line), 1, out);
      }
   }

   fclose(in);
   fclose(out);
   iconv_close(cd);

   if (unlink(argv[1]) < 0)
   {
      printf("Cannot remove original file (%s)\n", strerror(errno));
      return 7;
   }

   if (rename(outfile, argv[1]) != 0)
   {
      printf("Cannot rename temporary file (%s)\n", strerror(errno));
      return 8;
   }

   printf("%s\n", errors == 0 ? "OK" : "ERRORS");
   return 0;
}
