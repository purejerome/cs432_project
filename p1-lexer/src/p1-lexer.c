/**
 * @file p1-lexer.c
 * @brief Compiler phase 1: lexer
 */
#include "p1-lexer.h"

TokenQueue *
lex (const char *text)
{
  TokenQueue *tokens = TokenQueue_new ();
//"^(==|<=|>=|&&|\\|\\||[()\\+\\*{}\\[\\],;=\\-\\/<>!%])"
  /* compile regular expressions */
  Regex *whitespace = Regex_new ("^[ \n]");
  Regex *symbol = Regex_new("^(==|<=|>=|!=|&&|\\|\\||[][(){};=,+*\\-\\/%<>!])");
  Regex *decimal_int = Regex_new("^(0|[1-9][0-9]*)");
  Regex *identifier = Regex_new("^([a-z A-Z][a-zA-Z0-9]*)");
  Regex *string = Regex_new("^\"([^\"]*)\"");
  Regex *hex_literal = Regex_new("^(0x[0-9a-fA-F]+)");

  /* read and handle input */
  /* Read through decaf and understand program*/
  char match[MAX_TOKEN_LEN];
  int line_count = 1;
  while (*text != '\0')
    {
      /* match regular expressions */
      if (Regex_match (whitespace, text, match))
        {
          /* ignore whitespace */
          if(match[0] == '\n'){
            line_count = line_count + 1;
          }
        }
      else if (Regex_match (identifier, text, match))
        {
          /* TODO: implement line count and replace placeholder (-1) */
          TokenQueue_add (tokens, Token_new (ID, match, line_count));
        }
      else if (Regex_match (symbol, text, match))
        {
          /* TODO: implement line count and replace placeholder (-1) */
          TokenQueue_add (tokens, Token_new (SYM, match, line_count));
        }
      else if (Regex_match(hex_literal, text, match) || Regex_match (decimal_int, text, match))
        {
          bool hex_match = Regex_match(hex_literal, text, match);
          TokenQueue_add (tokens, Token_new (hex_match ? HEXLIT : DECLIT, match, line_count));
        }
      else if (Regex_match (string, text, match))
        {
          /* TODO: implement line count and replace placeholder (-1) */
          TokenQueue_add (tokens, Token_new (STRLIT, match, line_count));
        }
      else
        {
          Error_throw_printf ("Invalid token!\n");
        }
      /* skip matched text to look for next token */
      text += strlen (match);
    }

  /* clean up */
  Regex_free (whitespace);
  Regex_free (identifier);
  Regex_free (symbol);
  Regex_free (decimal_int);

  return tokens;
}
