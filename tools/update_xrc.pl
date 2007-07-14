#!/usr/bin/perl

$mode = 0;
$level = 0;
$isFont = 0;

while(<>)
{
   chomp;
   $line = $_;
   if ($mode == 0)
   {
      if ($line =~ /^.*\<object class="wxFrame" name="delete_.*/)
      {
         $mode = 1;
         $level++;
         $line = "";
      }
   }
   elsif ($mode == 1)
   {
      if ($line =~ /^.*\<object.*\/\>.*/)
      {
      }
      elsif ($line =~ /^.*\<object.*/)
      {
         $level++;
      }
      elsif ($line =~ /^.*\<\/object.*/)
      {
         $level--;
         if ($level == 0)
         {
            $line = "";
            $mode = 0;
         }
      }
      elsif ($level == 1)
      {
         $line = "";
      }
   }
   if ($isFont == 0)
   {
      if ($line =~ /^.*\<font\>.*/)
      {
         $line = "";
         $isFont = 1;   
      }
   }
   else
   {
      if ($line =~ /^.*\<\/font\>.*/)
      {
         $isFont = 0;   
      }
      $line = "";
   }
   if ($line ne "")
   {
      print "$line\n";
   }
}
