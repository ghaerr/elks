/* 
 * Written by Gregory Haerr for the ELKS project.
 */
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

/* macro for matching environment name in string*/
#define ENVNAME(var,buf,len)    (memcmp(var,buf,len) == 0 && (buf)[len] == '=')

/* local data*/
static char **  putenv_environ = NULL;  /* ptr to any environment we allocated*/

/*
 * Put or delete a string from the global process environment
 *
 * 'NAME=value'	adds environment variable name with value
 * 'NAME'	deletes environent variable if exists
 */
int
putenv(char *var)
{
	char **	env;
	int	envp_count;
	int	envp_len;
	int	namelen;
	int 	heap_bytes;
	char **	newenv;
	char **	nextarg;
	char *	nextstr;
	char *	rp;

	/* figure environment variable name length*/
	if ( (rp = strchr(var, '=')) == NULL)
		namelen = strlen(var);
	else namelen = rp - var;

	/* count environment bytes*/
again:
	envp_len = 0;
	env = environ;
	while (*env) {
		/* check for variable in current environment*/
		if (ENVNAME(var, *env, namelen)) {

			/* match, delete it and copy remaining up*/
			while ( (env[0] = env[1]) != NULL)
				++env;

			/* if requested to delete, we're done*/
			if (rp == NULL)
				return 0;

			goto again;
		}
		envp_len += strlen(*env++) + 1;
	}

	if (rp == NULL) {	/* missing '=' - reject */
		errno = EINVAL;
		return -1;
	}
	envp_len += strlen(var) + 1;		/* new environment variable*/
	envp_count = env - environ + 2;		/* + 1 for NULL terminator*/
						/* + 1 for newly added var*/

	/* compute new environment allocation size*/
	heap_bytes = envp_count * sizeof(char *) + envp_len;

	/* allocate new environment*/
	if ( (newenv = malloc(heap_bytes)) == NULL) {
		errno = ENOMEM;
		return -1;
	}

	/* build new environment*/
	nextarg = newenv;
	nextstr = (char *)&newenv[envp_count];
	env = environ;
	while (*env) {
		*nextarg++ = nextstr;
		strcpy(nextstr, *env);
		nextstr += strlen(nextstr) + 1;
		++env;
	}

	/* add new variable*/
	strcpy(nextstr, var);
	*nextarg++ = nextstr;
	*nextarg = NULL;

	/* free previous environment*/
	if (putenv_environ)
		free(putenv_environ);

	/* set new global environment*/
	environ = putenv_environ = newenv;
	return 0;
}
