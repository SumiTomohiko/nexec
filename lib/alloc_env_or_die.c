#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <fsyscall/private/die.h>

#include <nexec/util.h>

struct env*
alloc_env_or_die()
{
	struct env *penv;

	penv = (struct env *)malloc(sizeof(*penv));
	if (penv == NULL)
		die(1, "cannot allocate memory for struct env");
	return (penv);
}
