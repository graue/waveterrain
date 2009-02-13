#include <stdlib.h>
#include <stdio.h> // Tgk
#include <string.h>
#include <math.h>
#include "mt.h"
#include "xm.h"
#include "expr.h"

// inspired by madsyn

#define PERSISTENCE 50

// is_ancestor(a, b) means is a an ancestor of b? is b a subtree of a?
static int is_ancestor(expr_t *a, expr_t *b)
{
	if (a->depthcount < b->depthcount || a->varcount < b->varcount)
		return 0;
	if (a->type != EXPR_OP)
		return 0;

	if (a->details.op.oper->nargs == 1)
		return is_ancestor(a->details.op.args[0], b);

	// operator with 2 args
	return is_ancestor(a->details.op.args[0], b)
		|| is_ancestor(a->details.op.args[1], b);
}

static void update_expr_counts(expr_t *expr)
{
	expr_t *arg;

	if (expr->type != EXPR_OP)
	{
		expr->depthcount = 0;
		expr->varcount = expr->type != EXPR_CONSTANT;
	}
	else
	{
		arg = expr->details.op.args[0];
		update_expr_counts(arg);
		expr->depthcount = arg->depthcount;
		expr->varcount = arg->varcount;

		if (expr->details.op.oper->nargs == 2)
		{
			arg = expr->details.op.args[1];
			update_expr_counts(arg);
			if (arg->depthcount > expr->depthcount)
				expr->depthcount = arg->depthcount;
			expr->varcount += arg->varcount;
		}
		expr->depthcount++;
	}
}

// recursively deep-copy an expression, all subtrees etc.
static expr_t *copy_expr(expr_t *orig)
{
	expr_t *new;

	new = xm(sizeof *new, 1);
	memcpy(new, orig, sizeof *new);
	if (new->type == EXPR_OP)
	{
		new->details.op.args[0] =
			copy_expr(orig->details.op.args[0]);
		if (orig->details.op.oper->nargs == 2)
		{
			new->details.op.args[1] =
				copy_expr(orig->details.op.args[1]);
		}
	}
	return new;
}

static expr_t *findsubtree(expr_t *expr, int depth)
{
	if (depth <= 0 || expr->type != EXPR_OP)
		return expr;

	if (expr->details.op.oper->nargs == 1 || mt_rand()%2 == 0)
		return findsubtree(expr->details.op.args[0], depth-1);
	return findsubtree(expr->details.op.args[1], depth-1);
}

static expr_t *randomleaf(expr_t *expr)
{
	if (expr->type != EXPR_OP)
		return expr;
	if (expr->details.op.oper->nargs == 1 || mt_rand()%2 == 0)
		return randomleaf(expr->details.op.args[0]);
	return randomleaf(expr->details.op.args[1]);
}

static void mut_swapsubtrees(expr_t *expr)
{
	expr_t *sub1, *sub2;
	expr_t tmp;
	int tries = PERSISTENCE;

	do
	{
		sub1 = findsubtree(expr, mt_rand() % expr->depthcount);
		sub2 = findsubtree(expr, mt_rand() % expr->depthcount);
	} while (tries-- > 0
		&& (is_ancestor(sub1, sub2) || is_ancestor(sub2, sub1)));

	// couldn't find suitable inputs? give up
	if (tries <= 0)
		return;

	memcpy(&tmp, sub1, sizeof tmp);
	memcpy(sub1, sub2, sizeof *sub1);
	memcpy(sub2, &tmp, sizeof *sub2);
	update_expr_counts(expr);
}

// copy sub1 onto sub2, clobbering what is now in sub2
// make sure the formula contains at least 2 variables not in sub2
// or deleting sub2 would make it boring
static void mut_copysubtree(expr_t *expr)
{
	expr_t *sub1, *sub2;
	int tries = PERSISTENCE;

	do
	{
		sub1 = findsubtree(expr, mt_rand() % expr->depthcount);
		sub2 = findsubtree(expr, mt_rand() % expr->depthcount);
	} while (tries-- > 0
		&& (is_ancestor(sub1, sub2) || is_ancestor(sub2, sub1)
		|| expr->varcount - sub2->varcount < 2));

	// couldn't find suitable inputs? give up
	if (tries <= 0)
		return;

	// since no parent links are available,
	// this is done in a weird way

	// free children of sub2
	if (sub2->type == EXPR_OP)
	{
		free_expr(sub2->details.op.args[0]);
		if (sub2->details.op.oper->nargs == 2)
			free_expr(sub2->details.op.args[1]);
	}

	// copy sub1 to sub2
	memcpy(sub2, sub1, sizeof *sub2);

	// clone any children
	if (sub1->type == EXPR_OP)
	{
		sub2->details.op.args[0] =
			copy_expr(sub1->details.op.args[0]);
		if (sub1->details.op.oper->nargs == 2)
		{
			sub2->details.op.args[1] =
				copy_expr(sub1->details.op.args[1]);
		}
	}

	update_expr_counts(expr);
}

static void mut_changeleaf(expr_t *expr)
{
	expr_t *leaf;
	float f;

	leaf = randomleaf(expr);
	if (leaf->type == EXPR_X)
		leaf->type = EXPR_Y;
	else if (leaf->type == EXPR_Y)
		leaf->type = EXPR_X;
	else if (leaf->type == EXPR_CONSTANT)
	{
		f = leaf->details.num;
		f *= pow(2, (mt_frand()*2-0.5));
	}
}

void mutate(expr_t *expr)
{
	int mutation;

	mutation = mt_rand() % 2;
	if (mutation == 0)
		mut_swapsubtrees(expr);
	else if (mutation == 1)
		mut_changeleaf(expr);
	else // XXX NOT USED - BUGGY
		mut_copysubtree(expr);
}
