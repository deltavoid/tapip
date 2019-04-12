/*
 * simple shell for net stack debug and monitor
 */
#include "lib.h"
#include "arp.h"


/* extern net stack command handlers */
extern void arpcache(int, char **);
extern void netdebug(int, char **);
extern void ifconfig(int, char **);
extern void stat(int, char **);
extern void route(int, char **);
extern void ping(int, char **);
extern void ping2(int, char **);
extern void snc(int, char **);



static char *get_arg(char **pp)
{
	char *ret, *p;
	ret = NULL;
	p = *pp;
	/* skip white space and fill \0 */
	while (isblank(*p)) {
		*p = '\0';
		p++;
	}
	if (*p == '\0')
		goto out;
	ret = p;
	/* skip normal character */
	while (*p && !isblank(*p))
		p++;
out:
	*pp = p;
	return ret;
}

static int parse_line(char *line, int len, char **argv)
{
	int argc;
	char *p, *pp;
	/* assert len >= 0 */
	if (len == 0)
		return 0;

	p = pp = line;
	argc = 0;
	while ((p = get_arg(&pp)) != NULL)
		argv[argc++] = p;
	return argc;
}


typedef void (*cmd_func_t)(int , char** );

void inner_shell(cmd_func_t func, char* linebuf, int linelen)
{
	char* argv[16];
	int argc;

	argc = parse_line(linebuf, linelen, argv);

	func(argc, argv);

}