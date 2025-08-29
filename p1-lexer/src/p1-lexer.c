/**
 * @file p1-lexer.c
 * @brief Compiler phase 1: lexer
 */
#include "p1-lexer.h"

TokenQueue *
lex (const char *text)
{
  TokenQueue *tokens = TokenQueue_new ();

  /* compile regular expressions */
  Regex *whitespace = Regex_new ("^[ \n]");
  Regex *symbol = Regex_new("^[()\\+\\*]");
  Regex *decimal_int = Regex_new("^(0 | [1-9][0-9]*)");
  Regex *identifier = Regex_new("^([a-z A-Z][a-z A-Z 0-9]*)");

  /* read and handle input */
  /* Read through decaf and understand program*/
  char match[MAX_TOKEN_LEN];
  while (*text != '\0')
    {

      /* match regular expressions */
      if (Regex_match (whitespace, text, match))
        {
          /* ignore whitespace */
        }
      else if (Regex_match (identifier, text, match))
        {
          /* TODO: implement line count and replace placeholder (-1) */
          printf("Found identifier: %s\n", match);
          TokenQueue_add (tokens, Token_new (ID, match, -1));
        }
      else if (Regex_match (symbol, text, match))
        {
          /* TODO: implement line count and replace placeholder (-1) */
          printf("Found symbol: %s\n", match);
          TokenQueue_add (tokens, Token_new (SYM, match, -1));
        }
      else if (Regex_match (decimal_int, text, match))
        {
          /* TODO: implement line count and replace placeholder (-1) */
          printf("Found decimal integer: %s\n", match);
          TokenQueue_add (tokens, Token_new (DECLIT, match, -1));
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
