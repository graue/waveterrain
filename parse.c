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

static int isignored(char c)
{
	return isspace(c) || c == '(' || c == ')';
}

// note: cannot be called recursively, returned token is in temp storage
static char *nexttoken(const char *str, int *skipped)
{
	static char buf[999];
	int len = 0;

	while (str[len] != '\0' && !isignored(str[len])
		&& len+1 < (int)sizeof buf)
	{
		len++;
	}

	memcpy(buf, str, len);
	buf[len] = '\0';
	if (skipped != NULL) *skipped = len;

	return buf;
}

#undef PI
#define PI 3.14159265359 /* close enough */

expr_t *parse(const char *str, int *skipped)
{
	const char *p = str;
	const char *token;
	expr_t *myexpr;
	expr_t *arg;
	int numskipped;
	int ix;

	// skip leading spaces
	while (*p != '\0' && isignored(*p))
		p++;

	token = nexttoken(p, &numskipped);
	p += numskipped;
	if (token[0] == '\0')
		return NULL;

	myexpr = xm(sizeof *myexpr, 1);
	myexpr->depthcount = 0;

	if (isdigit(token[0]))
	{
		myexpr->type = EXPR_CONSTANT;
		myexpr->details.num = atof(token);
		myexpr->varcount = 0;
#ifdef PARSE_DEBUG
		fprintf(stderr, " %f", myexpr->details.num);
#endif
		if (skipped != NULL) *skipped = p - str;
		return myexpr;
	}

	if (!strcmp(token, "pi"))
	{
		myexpr->type = EXPR_CONSTANT;
		myexpr->details.num = PI;
		myexpr->varcount = 0;
#ifdef PARSE_DEBUG
		fprintf(stderr, " %f", myexpr->details.num);
#endif
		if (skipped != NULL) *skipped = p - str;
		return myexpr;
	}

	if (!strcmp(token, "x"))
	{
		myexpr->type = EXPR_X;
		myexpr->varcount = 1;
#ifdef PARSE_DEBUG
		fprintf(stderr, " x");
#endif
		if (skipped != NULL) *skipped = p - str;
		return myexpr;
	}
	if (!strcmp(token, "y"))
	{
		myexpr->type = EXPR_Y;
		myexpr->varcount = 1;
#ifdef PARSE_DEBUG
		fprintf(stderr, " y");
#endif
		if (skipped != NULL) *skipped = p - str;
		return myexpr;
	}
	if (!strcmp(token, "time"))
	{
		myexpr->type = EXPR_T;
		myexpr->varcount = 1;
#ifdef PARSE_DEBUG
		fprintf(stderr, " t");
#endif
		if (skipped != NULL) *skipped = p - str;
		return myexpr;
	}

	for (ix = 0; ix < numopers; ix++)
	{
		if (!strcmp(token, opers[ix].name))
		{
			myexpr->type = EXPR_OP;
			myexpr->details.op.oper = &opers[ix];
#ifdef PARSE_DEBUG
		fprintf(stderr, " %s (", opers[ix].name);
#endif
			arg = parse(p, &numskipped);
			p += numskipped;
			if (arg == NULL)
			{
				free(myexpr);
				return NULL;
			}
			myexpr->details.op.args[0] = arg;
			myexpr->varcount = arg->varcount;
			myexpr->depthcount = arg->depthcount;
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
				myexpr->varcount += arg->varcount;
				if (arg->depthcount > myexpr->depthcount)
					myexpr->depthcount = arg->depthcount;
				myexpr->details.op.args[1] = arg;
			}
			myexpr->depthcount++;
#ifdef PARSE_DEBUG
			fprintf(stderr, " )");
#endif
			if (skipped != NULL) *skipped = p - str;
			return myexpr;
		}
	}

	fprintf(stderr, "unable to make sense of token: \"%s\"\n", token);
	free(myexpr);
	return NULL;
}
