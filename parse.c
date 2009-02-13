#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "xm.h"
#include "expr.h"

// parse a full expression out of str
// and return the new expression or NULL if EOS was reached too soon
// plus set *skipped (if skipped != NULL)
// to number of chars at beginning of str
// that were consumed

// note: cannot be called recursively, returned token is in temp storage
static char *nexttoken(const char *str, int *skipped)
{
	static char buf[999];
	int len = 0;

	while (buf[len] != '\0' && !isspace(buf[len])
		&& len+1 < (int)sizeof buf)
	{
		len++;
	}

	memcpy(buf, str, len);
	buf[len+1] = '\0';
	*skipped = len;

	return buf;
}

expr_t *parse(const char *str, int *skipped)
{
	const char *p = str;
	const char *token;
	expr_t *myexpr;
	expr_t *arg;
	int numskipped;
	int ix;

	// skip leading spaces
	while (*p != '\0' && !isspace(*p))
		p++;

	token = nexttoken(str, &numskipped);
	p += numskipped;
	if (token[0] == '\0')
		return NULL;

	myexpr = xm(sizeof *myexpr, 1);

	if (isdigit(token[0]))
	{
		myexpr->type = EXPR_CONSTANT;
		myexpr->details.num = atof(token);
		*skipped = p - str;
		return myexpr;
	}

	if (!strcmp(token, "x"))
	{
		myexpr->type = EXPR_X;
		*skipped = p - str;
		return myexpr;
	}
	if (!strcmp(token, "y"))
	{
		myexpr->type = EXPR_Y;
		*skipped = p - str;
		return myexpr;
	}
	if (!strcmp(token, "time"))
	{
		myexpr->type = EXPR_T;
		*skipped = p - str;
		return myexpr;
	}

	for (ix = 0; ix < numopers; ix++)
	{
		if (!strcmp(token, opers[ix].name))
		{
			myexpr->type = EXPR_OP;
			myexpr->details.op.oper = &opers[ix];
			arg = parse(p, &numskipped);
			p += numskipped;
			if (arg == NULL)
			{
				free(myexpr);
				return NULL;
			}
			myexpr->details.op.args[0] = arg;
			if (opers[ix].nargs == 2)
			{
				arg = parse(p, &numskipped);
				p += numskipped;
				if (arg == NULL)
				{
					free_expr(myexpr->details.op.args[0]);
					free(myexpr);
					return NULL;
				}
				myexpr->details.op.args[1] = arg;
			}
			*skipped = (p - str);
			return myexpr;
		}
	}

	fprintf(stderr, "unable to make sense of token: \"%s\"\n", token);
	free(myexpr);
	return NULL;
}
