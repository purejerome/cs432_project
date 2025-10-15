/**
 * @file p3-analysis.c
 * @brief Compiler phase 3: static analysis
 */
/*
AI assist: AI was used to help pinpoint and correct some edge cases.
*/

#include "p3-analysis.h"

// Function to make it easier to print
static const char *
type_name (DecafType t)
{
  switch (t)
    {
    case INT:
      return "int";
    case BOOL:
      return "bool";
    case STR:
      return "str";
    case VOID:
      return "void";
    default:
      return "unknown";
    }
}

/**
 * @brief State/data for static analysis visitor
 */
typedef struct AnalysisData
{
  /**
   * @brief List of errors detected
   */
  ErrorList *errors;

  /* BOILERPLATE: TODO: add any new desired state information (and clean it up
   * in AnalysisData_free) */
  FuncDeclNode *current_function;
  int loop_depth;

} AnalysisData;

/**
 * @brief Allocate memory for analysis data
 *
 * @returns Pointer to allocated structure
 */
AnalysisData *
AnalysisData_new (void)
{
  AnalysisData *data = (AnalysisData *)calloc (1, sizeof (AnalysisData));
  CHECK_MALLOC_PTR (data);
  data->errors = ErrorList_new ();
  data->current_function = NULL;
  data->loop_depth = false;
  return data;
}

/**
 * @brief Deallocate memory for analysis data
 *
 * @param data Pointer to the structure to be deallocated
 */
void
AnalysisData_free (AnalysisData *data)
{
  /* free everything in data that is allocated on the heap except the error
   * list; it needs to be returned after the analysis is complete */
  /* free "data" itself */
  free (data);
}

/**
 * @brief Macro for more convenient access to the data inside a @ref
 * AnalysisVisitor data structure
 */
#define DATA ((AnalysisData *)visitor->data)

/**
 * @brief Macro for more convenient access to the error list inside a
 * @ref AnalysisVisitor data structure
 */
#define ERROR_LIST (((AnalysisData *)visitor->data)->errors)

/**
 * @brief Wrapper for @ref lookup_symbol that reports an error if the symbol
 * isn't found
 *
 * @param visitor Visitor with the error list for reporting
 * @param node AST node to begin the search at
 * @param name Name of symbol to find
 * @returns The @ref Symbol if found, otherwise @c NULL
 */
Symbol *
lookup_symbol_with_reporting (NodeVisitor *visitor, ASTNode *node,
                              const char *name)
{
  Symbol *symbol = lookup_symbol (node, name);
  if (symbol == NULL)
    {
      ErrorList_printf (ERROR_LIST, "Symbol '%s' undefined on line %d", name,
                        node->source_line);
    }
  return symbol;
}

/**
 * @brief Macro for shorter storing of the inferred @c type attribute
 */
#define SET_INFERRED_TYPE(T)                                                  \
  ASTNode_set_printable_attribute (node, "type", (void *)(T),                 \
                                   type_attr_print, dummy_free)

bool
contains_element_string (char **arr, int size, char *val)
{
  for (int i = 0; i < size; i++)
    {
      if (strncmp (arr[i], val, MAX_ID_LEN) == 0)
        {
          return true;
        }
    }
  return false;
}

/**
 * @brief Macro for shorter retrieval of the inferred @c type attribute
 */
#define GET_INFERRED_TYPE(N)                                                  \
  (DecafType) (long) ASTNode_get_attribute (N, "type")

void
AnalysisVisitor_check_duplicate_symbols (NodeVisitor *visitor, ASTNode *node)
{
  /* Only run on scope-bearing nodes */
  if (node->type != PROGRAM && node->type != BLOCK)
    return;

  /* Determine the “scope start” line for the message.
     - PROGRAM: use its own line (usually 1)
     - BLOCK:   use the block’s line (this is the function body’s line for the
                top-level block inside a function) */
  int scope_line = node->source_line;

  /* Grab this scope’s symbol table (attribute key may vary in codebases) */
  SymbolTable *table
      = (SymbolTable *)ASTNode_get_attribute (node, "symbolTable");
  if (table == NULL)
    table = (SymbolTable *)ASTNode_get_attribute (node, "symbols");

  /* 1) Check duplicates among local symbols in this scope */
  if (table && table->local_symbols)
    {
      for (Symbol *s1 = table->local_symbols->head; s1; s1 = s1->next)
        {

          /* skip names we’ve already reported earlier in this scope */
          bool seen_before = false;
          for (Symbol *p = table->local_symbols->head; p != s1; p = p->next)
            {
              if (strncmp (p->name, s1->name, MAX_ID_LEN) == 0)
                {
                  seen_before = true;
                  break;
                }
            }
          if (seen_before)
            continue;

          /* look ahead for a second occurrence of the same name */
          bool dup = false;
          for (Symbol *s2 = s1->next; s2; s2 = s2->next)
            {
              if (strncmp (s1->name, s2->name, MAX_ID_LEN) == 0)
                {
                  dup = true;
                  break;
                }
            }

          if (dup)
            {
              ErrorList_printf (
                  ERROR_LIST,
                  "Duplicate symbols named '%s' in scope started on line %d",
                  s1->name, scope_line);
            }
        }
    }

  /* 2) If this is the *function body* block, also check parameter names.
        (We can detect the function body by comparing the current block node to
         DATA->current_function->body.) */
  if (node->type == BLOCK && DATA->current_function != NULL
      && DATA->current_function->body == node
      && DATA->current_function->parameters != NULL)
    {
      ParameterList *plist = DATA->current_function->parameters;

      for (Parameter *p1 = plist->head; p1; p1 = p1->next)
        {

          /* skip names already seen earlier in the parameter list */
          bool seen_before = false;
          for (Parameter *q = plist->head; q != p1; q = q->next)
            {
              if (strncmp (q->name, p1->name, MAX_ID_LEN) == 0)
                {
                  seen_before = true;
                  break;
                }
            }
          if (seen_before)
            continue;

          /* look ahead for a second occurrence */
          bool dup = false;
          for (Parameter *p2 = p1->next; p2; p2 = p2->next)
            {
              if (strncmp (p1->name, p2->name, MAX_ID_LEN) == 0)
                {
                  dup = true;
                  break;
                }
            }

          if (dup)
            {
              ErrorList_printf (
                  ERROR_LIST,
                  "Duplicate symbols named '%s' in scope started on line %d",
                  p1->name, scope_line);
            }
        }
    }
}

void
AnalysisVisitor_check_main_function (NodeVisitor *visitor, ASTNode *node)
{
  Symbol *symbol = lookup_symbol (node, "main");
  if (symbol == NULL)
    {
      ErrorList_printf (ERROR_LIST,
                        "Program does not contain a 'main' function");
    }
  else
    {
      if (symbol->symbol_type != FUNCTION_SYMBOL)
        {
          ErrorList_printf (ERROR_LIST,
                            "Symbol 'main' is not a function on line %d",
                            node->source_line);
        }
      if (symbol->type != INT)
        {
          ErrorList_printf (ERROR_LIST, "'main' must return an integer");
        }
      if (symbol->parameters != NULL
          && ParameterList_size (symbol->parameters) != 0)
        {
          ErrorList_printf (ERROR_LIST, "'main' must take no parameters");
        }
    }
  return;
}

void
AnalysisVisitor_check_conditional (NodeVisitor *visitor, ASTNode *node)
{
  DecafType cond_type = GET_INFERRED_TYPE (node->conditional.condition);
  if (cond_type == UNKNOWN)
    return;

  if (cond_type != BOOL)
    {
      ErrorList_printf (ERROR_LIST,
                        "Type mismatch: bool expected but %s found on line %d",
                        type_name (cond_type), node->source_line);
    }
}

void
AnalysisVisitor_check_while (NodeVisitor *visitor, ASTNode *node)
{
  DecafType condition_type = GET_INFERRED_TYPE (node->whileloop.condition);
  if (condition_type != BOOL)
    {
      ErrorList_printf (
          ERROR_LIST,
          "Type error on line %d: while loop condition is not boolean",
          node->source_line);
    }
  if (DATA->loop_depth > 0)
    DATA->loop_depth--;
  return;
}

void
AnalysisVisitor_infer_literal (NodeVisitor *visitor, ASTNode *node)
{
  SET_INFERRED_TYPE (node->literal.type);
}

void
AnalysisVisitor_check_assignment (NodeVisitor *visitor, ASTNode *node)
{
  DecafType left_type = GET_INFERRED_TYPE (node->assignment.location);
  DecafType right_type = GET_INFERRED_TYPE (node->assignment.value);

  if (left_type == UNKNOWN || right_type == UNKNOWN)
    return;

  if (left_type != right_type)
    {
      ErrorList_printf (
          ERROR_LIST, "Type mismatch: %s is incompatible with %s on line %d",
          type_name (left_type), type_name (right_type), node->source_line);
    }
}

void
AnalysisVisitor_check_vardecl (NodeVisitor *visitor, ASTNode *node)
{
  DecafType var_type = node->vardecl.type;
  if (var_type == VOID)
    {
      ErrorList_printf (ERROR_LIST, "Void variable '%s' on line %d",
                        node->vardecl.name, node->source_line);
    }
  if (node->vardecl.is_array && node->vardecl.array_length <= 0)
    {
      ErrorList_printf (
          ERROR_LIST,
          "Array '%s' on line %d must have positive non-zero length",
          node->vardecl.name, node->source_line);
    }
  if (node->vardecl.is_array && DATA->current_function != NULL)
    {
      ErrorList_printf (ERROR_LIST,
                        "Local variable '%s' on line %d cannot be an array",
                        node->vardecl.name, node->source_line);
    }
  return;
}

void
AnalysisVisitor_set_current_function_type (NodeVisitor *visitor, ASTNode *node)
{
  DATA->current_function = &node->funcdecl;
  return;
}

void
AnalysisVisitor_reset_current_function_type (NodeVisitor *visitor,
                                             ASTNode *node)
{
  DATA->current_function = NULL;
  return;
}

void
AnalysisVisitor_check_return (NodeVisitor *visitor, ASTNode *node)
{
  DecafType expr_type = (node->funcreturn.value == NULL)
                            ? VOID
                            : GET_INFERRED_TYPE (node->funcreturn.value);

  FuncDeclNode *fn = DATA->current_function;
  if (fn == NULL)
    return;

  if (node->funcreturn.value != NULL && expr_type == UNKNOWN)
    return;

  // 1) Special case: void function returning a value
  if (fn->return_type == VOID && node->funcreturn.value != NULL)
    {
      ErrorList_printf (
          ERROR_LIST, "Invalid non-void return from void function on line %d",
          node->source_line);
      return;
    }

  // 2) Non-void function: returning no value?
  if (fn->return_type != VOID && node->funcreturn.value == NULL)
    {
      ErrorList_printf (
          ERROR_LIST, "Missing return value for non-void function on line %d",
          node->source_line);
      return;
    }

  // 3) Regular mismatch (both sides present)
  if (expr_type != fn->return_type)
    {
      ErrorList_printf (ERROR_LIST,
                        "Type mismatch: %s expected but %s found on line %d",
                        type_name (fn->return_type), type_name (expr_type),
                        node->source_line);
    }
}

void
AnalysisVisitor_infer_funccall (NodeVisitor *visitor, ASTNode *node)
{
  Symbol *symbol
      = lookup_symbol_with_reporting (visitor, node, node->funccall.name);
  if (symbol != NULL)
    {
      SET_INFERRED_TYPE (symbol->type);
    }
  else
    {
      SET_INFERRED_TYPE (UNKNOWN);
    }
  return;
}

void
AnalysisVisitor_check_funccall (NodeVisitor *visitor, ASTNode *node)
{
  Symbol *func_symbol
      = lookup_symbol_with_reporting (visitor, node, node->funccall.name);
  if (func_symbol == NULL)
    {
      return;
    }

  if (func_symbol->symbol_type != FUNCTION_SYMBOL)
    {
      ErrorList_printf (ERROR_LIST,
                        "Type error on line %d: symbol '%s' is not a function",
                        node->source_line, node->funccall.name);
      return;
    }

  ParameterList *formal_params = func_symbol->parameters;
  int formal_count
      = (formal_params == NULL) ? 0 : ParameterList_size (formal_params);
  NodeList *arguments = node->funccall.arguments;
  int argument_count = (arguments == NULL) ? 0 : NodeList_size (arguments);

  if (formal_count != argument_count)
    {
      ErrorList_printf (ERROR_LIST,
                        "Invalid number of function arguments on line %d",
                        node->source_line);
      return;
    }

  /* Per-argument type checking: emit one line per mismatch (no early return).
   */
  if (formal_count > 0 && argument_count > 0)
    {
      Parameter *formal = formal_params->head;
      ASTNode *argument = arguments->head;
      int idx = 0;

      while (formal != NULL && argument != NULL)
        {
          DecafType expected = formal->type;
          DecafType actual = GET_INFERRED_TYPE (argument);

          /* Avoid cascades: if child's type is unknown, skip emitting a
           * mismatch. */
          if (expected != UNKNOWN && actual != UNKNOWN && expected != actual)
            {
              ErrorList_printf (ERROR_LIST,
                                "Type mismatch in parameter %d of call to "
                                "'%s': expected %s but found %s on line %d",
                                idx, node->funccall.name, type_name (expected),
                                type_name (actual), node->source_line);
            }

          formal = formal->next;
          argument = argument->next;
          idx++;
        }
    }
}

void
AnalysisVisitor_infer_location (NodeVisitor *visitor, ASTNode *node)
{
  Symbol *sym
      = lookup_symbol_with_reporting (visitor, node, node->location.name);

  if (!sym)
    {
      SET_INFERRED_TYPE (UNKNOWN);
      return;
    }

  SET_INFERRED_TYPE (sym->type);
}

void
AnalysisVisitor_check_location (NodeVisitor *visitor, ASTNode *node)
{
  Symbol *symbol = lookup_symbol (node, node->location.name);

  if (node->location.index != NULL)
    {
      if (symbol != NULL && symbol->symbol_type != ARRAY_SYMBOL)
        {
          ErrorList_printf (
              ERROR_LIST,
              "Type error on line %d: non-array variable used with index",
              node->source_line);
        }

      DecafType index_type = GET_INFERRED_TYPE (node->location.index);

      if (index_type == UNKNOWN)
        {
          SET_INFERRED_TYPE (UNKNOWN);
          return;
        }

      if (index_type != INT)
        {
          ErrorList_printf (
              ERROR_LIST,
              "Type mismatch: int expected but %s found on line %d",
              type_name (index_type), node->source_line);
          SET_INFERRED_TYPE (UNKNOWN);
          return;
        }
    }
  else
    {
      if (symbol != NULL && symbol->symbol_type != SCALAR_SYMBOL)
        {
          ErrorList_printf (ERROR_LIST,
                            "Array '%s' accessed without index on line %d",
                            node->location.name, node->source_line);
        }
    }
  return;
}
void
AnalysisVisitor_infer_unaryop (NodeVisitor *visitor, ASTNode *node)
{
  switch (node->unaryop.operator)
    {
    case NEGOP:
      SET_INFERRED_TYPE (INT);
      break;
    case NOTOP:
      SET_INFERRED_TYPE (BOOL);
      break;
    default:
      ErrorList_printf (ERROR_LIST,
                        "Internal error: unhandled unary operator on line %d",
                        node->source_line);
      SET_INFERRED_TYPE (UNKNOWN);
    }
  return;
}

void
AnalysisVisitor_check_unaryop (NodeVisitor *visitor, ASTNode *node)
{
  DecafType child_type = GET_INFERRED_TYPE (node->unaryop.child);

  switch (node->unaryop.operator)
    {
    case NEGOP:
      if (child_type != INT)
        {
          ErrorList_printf (
              ERROR_LIST,
              "Type mismatch: int expected but %s found on line %d",
              type_name (child_type), node->source_line);
        }
      break;
    case NOTOP:
      if (child_type != BOOL)
        {
          ErrorList_printf (
              ERROR_LIST,
              "Type mismatch: bool expected but %s found on line %d",
              type_name (child_type), node->source_line);
        }
      break;
    default:
      ErrorList_printf (ERROR_LIST,
                        "Internal error: unhandled unary operator on line %d",
                        node->source_line);
    }
  return;
}

void
AnalysisVisitor_infer_binaryop (NodeVisitor *visitor, ASTNode *node)
{
  switch (node->binaryop.operator)
    {
    case ADDOP:
    case SUBOP:
    case MULOP:
    case DIVOP:
    case MODOP:
      SET_INFERRED_TYPE (INT);
      break;
    case OROP:
    case ANDOP:
    case LTOP:
    case LEOP:
    case GEOP:
    case GTOP:
    case EQOP:
    case NEQOP:
      SET_INFERRED_TYPE (BOOL);
      break;
    default:
      ErrorList_printf (ERROR_LIST,
                        "Internal error: unhandled binary operator on line %d",
                        node->source_line);
      SET_INFERRED_TYPE (UNKNOWN);
    }
  return;
}

void
AnalysisVisitor_check_binaryop (NodeVisitor *visitor, ASTNode *node)
{
  DecafType left_type = GET_INFERRED_TYPE (node->binaryop.left);
  DecafType right_type = GET_INFERRED_TYPE (node->binaryop.right);

  if (left_type == UNKNOWN || right_type == UNKNOWN)
    {
      SET_INFERRED_TYPE (UNKNOWN);
      return;
    }

  switch (node->binaryop.operator)
    {
    case ADDOP:
    case SUBOP:
    case MULOP:
    case DIVOP:
    case MODOP:
    case LTOP:
    case LEOP:
    case GEOP:
    case GTOP:
      if (left_type != INT)
        {
          ErrorList_printf (
              ERROR_LIST,
              "Type mismatch: int expected but %s found on line %d",
              type_name (left_type), node->source_line);
        }
      if (right_type != INT)
        {
          ErrorList_printf (
              ERROR_LIST,
              "Type mismatch: int expected but %s found on line %d",
              type_name (right_type), node->source_line);
        }
      break;
    case OROP:
    case ANDOP:
      if (left_type != BOOL)
        {
          ErrorList_printf (
              ERROR_LIST,
              "Type mismatch: bool expected but %s found on line %d",
              type_name (left_type), node->source_line);
        }
      if (right_type != BOOL)
        {
          ErrorList_printf (
              ERROR_LIST,
              "Type mismatch: bool expected but %s found on line %d",
              type_name (right_type), node->source_line);
        }
      break;
    case EQOP:
    case NEQOP:
      if (left_type != right_type)
        {
          ErrorList_printf (
              ERROR_LIST,
              "Type mismatch: %s is incompatible with %s on line %d",
              type_name (left_type), type_name (right_type),
              node->source_line);
        }
      break;
    default:
      ErrorList_printf (ERROR_LIST,
                        "Internal error: unhandled binary operator on line %d",
                        node->source_line);
    }
  return;
}

void
AnalysisVisitor_set_loop_depth (NodeVisitor *visitor, ASTNode *node)
{
  DATA->loop_depth++;
  return;
}

void
AnalysisVisitor_reset_loop_depth (NodeVisitor *visitor, ASTNode *node)
{
  DATA->loop_depth = false;
  return;
}

void
AnalysisVisitor_check_break (NodeVisitor *visitor, ASTNode *node)
{
  if (DATA->loop_depth <= 0)
    {
      ErrorList_printf (ERROR_LIST, "Invalid 'break' outside loop on line %d",
                        node->source_line);
    }
  return;
}

void
AnalysisVisitor_check_continue (NodeVisitor *visitor, ASTNode *node)
{
  if (DATA->loop_depth <= 0)
    {
      ErrorList_printf (ERROR_LIST,
                        "Invalid 'continue' outside loop on line %d",
                        node->source_line);
    }
  return;
}

typedef struct
{
  int block_depth;
} ListVisitorsData;

#define DATA2 ((ListVisitorsData *)(visitor->data))

void
ListVariablesVisitor_previsit_program (NodeVisitor *visitor, ASTNode *node)
{
  DATA2->block_depth = 0;
}

void
ListVariablesVisitor_previsit_block (NodeVisitor *visitor, ASTNode *node)
{
  DATA2->block_depth += 1;
}

void
ListVariablesVisitor_postvisit_block (NodeVisitor *visitor, ASTNode *node)
{
  DATA2->block_depth -= 1;
}

void
ListVariablesVisitor_visit_vardecl (NodeVisitor *visitor, ASTNode *node)
{
  printf ("%d %s\n", DATA2->block_depth, node->vardecl.name);
}

NodeVisitor *
ListVariablesVisitor_new ()
{
  NodeVisitor *v = NodeVisitor_new ();
  v->data = malloc (sizeof (ListVisitorsData));
  v->dtor = free;
  v->previsit_program = ListVariablesVisitor_previsit_program;
  v->previsit_block = ListVariablesVisitor_previsit_block;
  v->postvisit_block = ListVariablesVisitor_postvisit_block;
  v->previsit_vardecl = ListVariablesVisitor_visit_vardecl;
  return v;
}

ErrorList *
analyze (ASTNode *tree)
{
  if (tree == NULL)
    {
      ErrorList *errors = ErrorList_new ();
      return errors;
    }

  NodeVisitor *v = NodeVisitor_new ();
  v->data = (void *)AnalysisData_new ();
  v->dtor = (Destructor)AnalysisData_free;

  /* ---- Visitor wiring ---- */

  // Scope duplicate checks AFTER scopes are populated
  v->postvisit_program = AnalysisVisitor_check_duplicate_symbols;
  v->postvisit_block = AnalysisVisitor_check_duplicate_symbols;
  // (No need to also attach on funcdecl; running on the body BLOCK covers it.)

  // Main function checks
  v->previsit_program = AnalysisVisitor_check_main_function;

  // While-loop context + checks
  v->previsit_whileloop = AnalysisVisitor_set_loop_depth;
  v->postvisit_whileloop = AnalysisVisitor_check_while;

  // Conditionals (if-statements)
  v->postvisit_conditional = AnalysisVisitor_check_conditional;

  // Break/continue
  v->postvisit_break = AnalysisVisitor_check_break;
  v->postvisit_continue = AnalysisVisitor_check_continue;

  // Literals and expressions
  v->previsit_literal = AnalysisVisitor_infer_literal;

  v->previsit_unaryop = AnalysisVisitor_infer_unaryop;
  v->postvisit_unaryop = AnalysisVisitor_check_unaryop;

  v->previsit_binaryop = AnalysisVisitor_infer_binaryop;
  v->postvisit_binaryop = AnalysisVisitor_check_binaryop;

  // Locations (variables / array refs)
  v->previsit_location = AnalysisVisitor_infer_location;
  v->postvisit_location = AnalysisVisitor_check_location;

  // Assignments
  v->postvisit_assignment = AnalysisVisitor_check_assignment;

  // Variable declarations
  v->previsit_vardecl = AnalysisVisitor_check_vardecl;

  // Function context
  v->previsit_funcdecl = AnalysisVisitor_set_current_function_type;
  v->postvisit_funcdecl = AnalysisVisitor_reset_current_function_type;

  // Function calls
  v->previsit_funccall = AnalysisVisitor_infer_funccall;
  v->postvisit_funccall = AnalysisVisitor_check_funccall;

  // Returns
  v->postvisit_return = AnalysisVisitor_check_return;

  /* ---- Traverse, collect, return ---- */
  NodeVisitor_traverse (v, tree);
  ErrorList *errors = ((AnalysisData *)v->data)->errors;
  NodeVisitor_free (v);
  return errors;
}
