#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "expr.h"

// primitives that are not already functions
float add(float a, float b) { return a + b; }
float sub(float a, float b) { return a - b; }
float mul(float a, float b) { return a * b; }
float div(float a, float b) { return b == 0.0 ? 0.0 : a / b; }
float mod(float a, float b) { return b == 0.0 ? 0.0 : fmodf(a, b); }

const operator_t opers[] =
{
	{ "+", 2, { .func2 = add }},
	{ "-", 2, { .func2 = sub }},
	{ "*", 2, { .func2 = mul }},
	{ "/", 2, { .func2 = div }},
	{ "%", 2, { .func2 = mod }},
	{ "sin", 1, { .func1 = sinf }},
	{ "cos", 1, { .func1 = cosf }}
};

const int numopers = sizeof opers / sizeof opers[0];

float evaluate(expr_t *expr, float x, float y, float t)
{
	float f1, f2;
	const operator_t *oper;

	if (expr->type == EXPR_CONSTANT)
		return expr->details.num;
	if (expr->type == EXPR_X)
		return x;
	if (expr->type == EXPR_Y)
		return y;
	if (expr->type == EXPR_T)
		return t;

	assert(expr->type == EXPR_OP);

	f1 = evaluate(expr->details.op.args[0], x, y, t);

	oper = expr->details.op.oper;
	if (oper->nargs == 1)
		return oper->func.func1(f1);

	f2 = evaluate(expr->details.op.args[1], x, y, t);
	return oper->func.func2(f1, f2);
}

void free_expr(expr_t *expr)
{
	if (expr->type == EXPR_OP)
	{
		free_expr(expr->details.op.args[0]);
		if (expr->details.op.oper->nargs == 2)
			free_expr(expr->details.op.args[1]);
	}
	free(expr);
}
