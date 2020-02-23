#ifdef L_getgid
int getgid(void)
{
   int egid;
   return __getgid(&egid);
}
#endif
