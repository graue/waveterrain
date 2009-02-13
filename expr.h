typedef float (*op1_t)(float);
typedef float (*op2_t)(float, float);

typedef struct
{
	const char *name;
	int nargs; // 1 or 2
	union {
		op1_t func1;
		op2_t func2;
	} func;
} operator_t;

extern const operator_t opers[];
extern const int numopers;

// an expression can be:
//   a constant number
//   a variable (x, y, or t)
//   an operator with 1 or 2 expression(s) as argument(s)
enum { EXPR_CONSTANT, EXPR_X, EXPR_Y, EXPR_T, EXPR_OP };

typedef struct expr_s
{
	int type;
	int varcount; // number of variables in this and all subtrees
	int depthcount; // depth of deepest leaf in distance from this node
	union {
		float num; // EXPR_CONSTANT
		struct {
			const operator_t *oper;
			struct expr_s *args[2];
		} op; // EXPR_OP
	} details;
} expr_t;

float evaluate(expr_t *expr, float x, float y, float t);
void free_expr(expr_t *expr);
