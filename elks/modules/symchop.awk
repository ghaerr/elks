BEGIN{
   print ".text\n.ts:";
   print "entry .ts"
   print ".data\n.ds:";
}
{
   lname=$2;
   seg=$3;
   off=substr($4,5,4);
   if( $5 != "R" ) next;

   if( substr(lname, 1, 1) != "_" ) next;
   if( substr(lname, 2, 1) == "_" ) next;

   if( lname in done ) next;
   done[lname] = 1;

   if( lname == "_module_init" || lname == "_module_data" )
   {
      print "int",lname "=0x" off,";" > "symchop.pos";
   }

   if( seg == 0 )
   {
      print "export", lname;
      print lname "= .ts+0x" off;
   }
   if( seg == 3 )
   {
      print "export", lname;
      print lname "= .ds+0x" off;
   }
}
END{
   print ".text"
   print ".blkb _module_init-.ts"
   print ".data"
   print ".blkb _module_data-.ds"
}
