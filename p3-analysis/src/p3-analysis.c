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
  SymbolTable *table
      = (SymbolTable *)ASTNode_get_attribute (node, "symbolTable");
  if (table != NULL)
    {
      int count = SymbolList_size (table->local_symbols);
      char **names = malloc (count * sizeof (char *));
      int dup_count = 0;
      FOR_EACH (Symbol *, sym, table->local_symbols)
      {
        Symbol *other = SymbolTable_lookup (table, sym->name);
        if (other != NULL && other != sym
            && !contains_element_string (names, dup_count, sym->name))
          {
            names[dup_count] = sym->name;
            dup_count = dup_count + 1;
            ErrorList_printf (
                ERROR_LIST,
                "Duplicate symbols named '%s' in scope started on line %d",
                sym->name, node->source_line);
          }
      }
      // When i comment this out its not affecting any tests?
      // for (int i = 0; i < dup_count; i++)
      //   {
      //     FOR_EACH (Symbol *, sym, table->local_symbols)
      //     {
      //       if (strncmp (sym->name, names[i], MAX_ID_LEN) == 0)
      //         {
      //           sym->type = UNKNOWN;
      //         }
      //     }
      //   }
      free (names);
    }
  return;
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
  DecafType cond_type = GET_INFERRED_TYPE (node->whileloop.condition);
  if (cond_type != BOOL)
    {
      ErrorList_printf (ERROR_LIST,
                        "Type mismatch: bool expected but %s found on line %d",
                        type_name (cond_type), node->source_line);
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
  // AnalysisVisitor_check_duplicate_symbols (visitor, node);
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
  Symbol *fn_symbol = lookup_symbol (node, fn->name);
  DecafType fn_type = (fn_symbol == NULL) ? UNKNOWN : fn_symbol->type;

  if (fn_type == UNKNOWN)
    return;

  if (node->funcreturn.value != NULL && expr_type == UNKNOWN)
    return;

  // 1) Special case: void function returning a value
  if (fn_type == VOID && node->funcreturn.value != NULL)
    {
      ErrorList_printf (
          ERROR_LIST, "Invalid non-void return from void function on line %d",
          node->source_line);
      return;
    }

  // 2) Non-void function: returning no value?
  if (fn_type != VOID && node->funcreturn.value == NULL)
    {
      ErrorList_printf (
          ERROR_LIST, "Invalid void return from non-void function on line %d",
          node->source_line);
      return;
    }

  // 3) Regular mismatch (both sides present)
  if (expr_type != fn_type)
    {
      ErrorList_printf (
          ERROR_LIST, "Type mismatch: %s expected but %s found on line %d",
          type_name (fn_type), type_name (expr_type), node->source_line);
    }
}

void
AnalysisVisitor_infer_funccall (NodeVisitor *visitor, ASTNode *node)
{
  Symbol *symbol
      = lookup_symbol_with_reporting (visitor, node, node->funccall.name);

  if (symbol != NULL && symbol->symbol_type == FUNCTION_SYMBOL)
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
  Symbol *func_symbol = lookup_symbol (node, node->funccall.name);

  if (func_symbol == NULL)
    {
      SET_INFERRED_TYPE (UNKNOWN);
      return;
    }

  if (func_symbol->symbol_type != FUNCTION_SYMBOL)
    {
      ErrorList_printf (ERROR_LIST,
                        "Invalid call to non-function '%s' on line %d",
                        node->funccall.name, node->source_line);
      SET_INFERRED_TYPE (UNKNOWN);
      // return; // <-- THIS return is crucial
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
      SET_INFERRED_TYPE (UNKNOWN);
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
          ErrorList_printf (ERROR_LIST,
                            "Non-array '%s' accessed as an array on line %d",
                            node->location.name, node->source_line);
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
          if (symbol->symbol_type == ARRAY_SYMBOL)
            {
              ErrorList_printf (ERROR_LIST,
                                "Array '%s' accessed without index on line %d",
                                node->location.name, node->source_line);
            }
          else
            {
              ErrorList_printf (
                  ERROR_LIST,
                  "Function '%s' accessed as a variable on line %d",
                  node->location.name, node->source_line);
            }
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

// Run program-level checks once, in the desired order.
static void
AnalysisVisitor_finalize_program (NodeVisitor *visitor, ASTNode *node)
{
  // 1) Program/global-scope duplicates
  AnalysisVisitor_check_duplicate_symbols (visitor, node);
  // 2) 'main' signature/return checks
  AnalysisVisitor_check_main_function (visitor, node);
}

static void
AnalysisVisitor_postvisit_funcdecl_chain (NodeVisitor *visitor, ASTNode *node)
{
  // Catch duplicates in the *function-declaration scope* (parameters live
  // here)
  AnalysisVisitor_check_duplicate_symbols (visitor, node);

  // Keep your original teardown
  AnalysisVisitor_reset_current_function_type (visitor, node);
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
  // v->previsit_program = AnalysisVisitor_check_duplicate_symbols;
  // v->previsit_block = AnalysisVisitor_check_duplicate_symbols;
  v->postvisit_block = AnalysisVisitor_check_duplicate_symbols;

  // (No need to also attach on funcdecl; running on the body BLOCK covers it.)

  // Main function checks
  // v->postvisit_program = AnalysisVisitor_check_main_function;
  v->postvisit_program = AnalysisVisitor_finalize_program;

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
  // v->postvisit_funcdecl = AnalysisVisitor_reset_current_function_type;
  v->postvisit_funcdecl = AnalysisVisitor_postvisit_funcdecl_chain;

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
