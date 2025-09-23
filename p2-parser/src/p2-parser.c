/**
 * @file p2-parser.c
 * @brief Compiler phase 2: parser
 */

#include "p2-parser.h"

ASTNode *parse (TokenQueue *input);
ASTNode *parse_program (TokenQueue *input);
ASTNode *parse_vardecl (TokenQueue *input);
ASTNode *parse_var_or_func (TokenQueue *input);
ASTNode *parse_funcdecl (TokenQueue *input);
ASTNode *parse_block (TokenQueue *input);
ASTNode *parse_statement (TokenQueue *input);
ASTNode *parse_literal (TokenQueue *input);
ASTNode *parse_base_expression (TokenQueue *input);
ParameterList *parse_params (TokenQueue *input);
DecafType parse_type (TokenQueue *input);
void parse_id (TokenQueue *input, char *buffer);
/*
 * IMPROTANT TO IMPLEMENT, SPLIT THE LOC AND FUNC CALL
 * PARSING INTO TWO SECTIONS CAUSE THEY BOTH BEGIN WITH ID.
 */

/*
 * helper functions
 */

/**
 * @brief Look up the source line of the next token in the queue.
 *
 * @param input Token queue to examine
 * @returns Source line
 */
int
get_next_token_line (TokenQueue *input)
{
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf ("Unexpected end of input\n");
    }
  return TokenQueue_peek (input)->line;
}

/**
 * @brief Check next token for a particular type and text and discard it
 *
 * Throws an error if there are no more tokens or if the next token in the
 * queue does not match the given type or text.
 *
 * @param input Token queue to modify
 * @param type Expected type of next token
 * @param text Expected text of next token
 */
void
match_and_discard_next_token (TokenQueue *input, TokenType type,
                              const char *text)
{
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf ("Unexpected end of input (expected \'%s\')\n", text);
    }
  Token *token = TokenQueue_remove (input);
  if (token->type != type || !token_str_eq (token->text, text))
    {
      Error_throw_printf ("Expected \'%s\' but found '%s' on line %d\n", text,
                          token->text, get_next_token_line (input));
    }
  Token_free (token);
}

/**
 * @brief Remove next token from the queue
 *
 * Throws an error if there are no more tokens.
 *
 * @param input Token queue to modify
 */
void
discard_next_token (TokenQueue *input)
{
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf ("Unexpected end of input\n");
    }
  Token_free (TokenQueue_remove (input));
}

/**
 * @brief Look ahead at the type of the next token
 *
 * @param input Token queue to examine
 * @param type Expected type of next token
 * @returns True if the next token is of the expected type, false if not
 */
bool
check_next_token_type (TokenQueue *input, TokenType type)
{
  if (TokenQueue_is_empty (input))
    {
      return false;
    }
  Token *token = TokenQueue_peek (input);
  return (token->type == type);
}

/**
 * @brief Look ahead at the type and text of the next token
 *
 * @param input Token queue to examine
 * @param type Expected type of next token
 * @param text Expected text of next token
 * @returns True if the next token is of the expected type and text, false if
 * not
 */
bool
check_next_token (TokenQueue *input, TokenType type, const char *text)
{
  if (TokenQueue_is_empty (input))
    {
      return false;
    }
  Token *token = TokenQueue_peek (input);
  return (token->type == type) && (token_str_eq (token->text, text));
}

ASTNode *
parse_literal (TokenQueue *input)
{
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf ("Unexpected end of input (expected literal)\n");
    }
  int source_line = get_next_token_line (input);
  if (check_next_token_type (input, DECLIT))
    {
      Token *int_token = TokenQueue_remove (input);
      int value = strtol (int_token->text, NULL, 10);
      Token_free (int_token);
      return LiteralNode_new_int (value, source_line);
    }
  else if (check_next_token_type (input, HEXLIT))
    {
      Token *hex_token = TokenQueue_remove (input);
      int value = strtol (hex_token->text, NULL, 16);
      Token_free (hex_token);
      return LiteralNode_new_int (value, source_line);
    }
  else if (check_next_token_type (input, STRLIT))
    {
      Token *str_token = TokenQueue_remove (input);
      char string_value[MAX_LINE_LEN];
      strncpy (string_value, str_token->text, MAX_LINE_LEN);
      Token_free (str_token);
      return LiteralNode_new_string (string_value, source_line);
    }
  else if (check_next_token (input, KEY, "true"))
    {
      discard_next_token (input);
      return LiteralNode_new_bool (true, source_line);
    }
  else if (check_next_token (input, KEY, "false"))
    {
      discard_next_token (input);
      return LiteralNode_new_bool (false, source_line);
    }

  Error_throw_printf ("Error in literal layer.\n");
  return NULL;
}

ASTNode *
parse_base_expression (TokenQueue *input)
{
  // change this later to allow for more than just literals
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf ("Unexpected end of input (expected statement)\n");
    }
  if (check_next_token (input, SYM, "("))
    {
      discard_next_token (input);
      ASTNode *expr = NULL; // IMPLEMENTS THIS TO USE EXPR PARSING LATER
      match_and_discard_next_token (input, SYM, ")");
      return expr;
    }
  else
    {
      return parse_literal (input);
    }
}

ASTNode *
parse_statement (TokenQueue *input)
{
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf ("Unexpected end of input (expected statement)\n");
    }

  int source_line = get_next_token_line (input);
  if (check_next_token_type (input, KEY))
    {
      if (check_next_token (input, KEY, "return"))
        {
          discard_next_token (input);
          if (check_next_token (input, SYM, ";"))
            {
              match_and_discard_next_token (input, SYM, ";");
              return ReturnNode_new (NULL, source_line);
            }
          else
            {
              // change this later to allow from expr and not just base expr
              ASTNode *return_value = parse_base_expression (input);
              match_and_discard_next_token (input, SYM, ";");
              return ReturnNode_new (return_value, source_line);
            }
        }
      else if (check_next_token (input, KEY, "break"))
        {
          discard_next_token (input);
          match_and_discard_next_token (input, SYM, ";");
          return BreakNode_new (source_line);
        }
      else if (check_next_token (input, KEY, "continue"))
        {
          discard_next_token (input);
          match_and_discard_next_token (input, SYM, ";");
          return ContinueNode_new (source_line);
        }
    }
  Error_throw_printf ("Error in statement layer.\n");
  return NULL;
}

ASTNode *
parse_block (TokenQueue *input)
{
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf ("Unexpected end of input (expected block)\n");
    }

  int source_line = get_next_token_line (input);
  match_and_discard_next_token (input, SYM, "{");
  NodeList *vars = NodeList_new ();
  NodeList *stmts = NodeList_new ();
  while (!check_next_token (input, SYM, "}"))
    {
      if (check_next_token (input, KEY, "int")
          || check_next_token (input, KEY, "bool")
          || check_next_token (input, KEY, "void"))
        {
          ASTNode *var = parse_vardecl (input);
          NodeList_add (vars, var);
        }
      else
        {
          ASTNode *stmt = parse_statement (input);
          NodeList_add (stmts, stmt);
        }
      if (TokenQueue_is_empty (input))
        {
          Error_throw_printf ("Unexpected end of input (expected '}' to close "
                              "block started on line %d)\n",
                              source_line);
        }
    }
  match_and_discard_next_token (input, SYM, "}");
  return BlockNode_new (vars, stmts, source_line);
}

ASTNode *
parse_var_or_func (TokenQueue *input)
{
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf (
          "Unexpected end of input (expected variable declaration)\n");
    }

  bool is_func_decl = check_next_token (input, KEY, "def");

  if (is_func_decl)
    {
      return parse_funcdecl (input);
    }
  else
    {
      return parse_vardecl (input);
    }
}

ParameterList *
parse_params (TokenQueue *input)
{
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf ("Unexpected end of input (parameters)\n");
    }
  ParameterList *params = ParameterList_new ();
  DecafType type = parse_type (input);
  char id[MAX_TOKEN_LEN];
  parse_id (input, id);
  ParameterList_add_new (params, id, type);

  while (check_next_token (input, SYM, ","))
    {
      if (check_next_token (input, SYM, ","))
        {
          discard_next_token (input);
        }
      DecafType type = parse_type (input);
      char id[MAX_TOKEN_LEN];
      parse_id (input, id);
      ParameterList_add_new (params, id, type);
    }
  // if(check_next_token_type(input, KEY)){
  //     DecafType type = parse_type(input);
  //     char id[MAX_TOKEN_LEN];
  //     parse_id(input, id);
  //     ParameterList_add_new(params, id, type);

  //     while(check_next_token(input, SYM, ",")){
  //         if(check_next_token(input, SYM, ",")) {
  //             discard_next_token(input);
  //         }
  //         DecafType type = parse_type(input);
  //         char id[MAX_TOKEN_LEN];
  //         parse_id(input, id);
  //         ParameterList_add_new(params, id, type);
  //     }
  // }
  // while(!check_next_token(input, SYM, ")")) {
  // if(check_next_token(input, SYM, ",")) {
  //     discard_next_token(input);
  // }
  //     DecafType type = parse_type(input);
  //     char id[MAX_TOKEN_LEN];
  //     parse_id(input, id);
  //     ParameterList_add_new(params, id, type);
  // }
  return params;
}

ASTNode *
parse_funcdecl (TokenQueue *input)
{
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf ("Unexpected end of input (expected type)\n");
    }
  int source_line = get_next_token_line (input);
  match_and_discard_next_token (input, KEY, "def");
  DecafType type = parse_type (input);
  char id[MAX_TOKEN_LEN];
  parse_id (input, id);
  match_and_discard_next_token (input, SYM, "(");
  ParameterList *params;
  // if(check_next_token_type(input, KEY)) {
  //     params = parse_params(input);
  // } else {
  //     params = ParameterList_new();
  // }
  if (check_next_token (input, SYM, ")"))
    {
      params = ParameterList_new ();
    }
  else
    {
      params = parse_params (input);
    }
  match_and_discard_next_token (input, SYM, ")");
  ASTNode *block = parse_block (input);
  return FuncDeclNode_new (id, type, params, block, source_line);
}

ASTNode *
parse_vardecl (TokenQueue *input)
{
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf ("Unexpected end of input (expected type)\n");
    }

  int source_line = get_next_token_line (input);
  int array_length = 1;
  bool is_array = false;
  DecafType type = parse_type (input);
  char id[MAX_TOKEN_LEN];
  parse_id (input, id);
  if (check_next_token (input, SYM, "["))
    {
      is_array = true;
      discard_next_token (input);
      if (check_next_token_type (input, DECLIT))
        {
          Token *array_length_token = TokenQueue_remove (input);
          array_length = strtol (array_length_token->text, NULL, 10);
          Token_free (array_length_token);
        }
      else
        {
          Error_throw_printf ("Invalid array length on line %d\n",
                              get_next_token_line (input));
        }
      match_and_discard_next_token (input, SYM, "]");
    }
  match_and_discard_next_token (input, SYM, ";");

  return VarDeclNode_new (id, type, is_array, array_length, source_line);
}

/**
 * @brief Parse and return a Decaf type
 *
 * @param input Token queue to modify
 * @returns Parsed type (it is also removed from the queue)
 */
DecafType
parse_type (TokenQueue *input)
{
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf ("Unexpected end of input (expected type)\n");
    }
  Token *token = TokenQueue_remove (input);
  if (token->type != KEY)
    {
      Error_throw_printf ("Invalid type '%s' on line %d\n", token->text,
                          get_next_token_line (input));
    }
  DecafType t = VOID;
  if (token_str_eq ("int", token->text))
    {
      t = INT;
    }
  else if (token_str_eq ("bool", token->text))
    {
      t = BOOL;
    }
  else if (token_str_eq ("void", token->text))
    {
      t = VOID;
    }
  else
    {
      Error_throw_printf ("Invalid type '%s' on line %d\n", token->text,
                          get_next_token_line (input));
    }
  Token_free (token);
  return t;
}

/**
 * @brief Parse and return a Decaf identifier
 *
 * @param input Token queue to modify
 * @param buffer String buffer for parsed identifier (should be at least
 * @c MAX_TOKEN_LEN characters long)
 */
void
parse_id (TokenQueue *input, char *buffer)
{
  if (TokenQueue_is_empty (input))
    {
      Error_throw_printf ("Unexpected end of input (expected identifier)\n");
    }
  Token *token = TokenQueue_remove (input);
  if (token->type != ID)
    {
      Error_throw_printf ("Invalid ID '%s' on line %d\n", token->text,
                          get_next_token_line (input));
    }
  snprintf (buffer, MAX_ID_LEN, "%s", token->text);
  Token_free (token);
}

/*
 * node-level parsing functions
 */

ASTNode *
parse_program (TokenQueue *input)
{
  NodeList *vars = NodeList_new ();
  NodeList *funcs = NodeList_new ();
  // ASTNode* node = parse_literal(input);
  // NodeList_add(vars, node);
  while (!TokenQueue_is_empty (input))
    {
      ASTNode *node = parse_var_or_func (input);
      if (node->type == VARDECL)
        {
          NodeList_add (vars, node);
        }
      else
        {
          NodeList_add (funcs, node);
        }
    }
  printf ("Finished parsing program.\n");
  printf ("There are %d global variables and %d functions.\n",
          NodeList_size (vars), NodeList_size (funcs));
  return ProgramNode_new (vars, funcs);
}

ASTNode *
parse (TokenQueue *input)
{
  printf ("Starting parse...\n");
  if (input == NULL)
    {
      Error_throw_printf ("Input token queue is NULL\n");
    }
  return parse_program (input);
}
