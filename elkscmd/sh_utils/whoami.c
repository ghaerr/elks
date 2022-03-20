#include <unistd.h>
#include <string.h>
#include <pwd.h>

int main (void)
{
    register struct passwd *pw;
    register uid_t uid;
    
    uid = geteuid ();
    pw = getpwuid (uid);
    if (pw) {
	write(STDOUT_FILENO,pw->pw_name,strlen(pw->pw_name));
	write(STDOUT_FILENO,"\n",1);
	return 0;
    }
    return 1;
}
