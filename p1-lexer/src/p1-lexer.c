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
  Regex *letter = Regex_new ("^[a-z]");

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
      else if (Regex_match (letter, text, match))
        {
          /* TODO: implement line count and replace placeholder (-1) */
          TokenQueue_add (tokens, Token_new (ID, match, -1));
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
  Regex_free (letter);

  return tokens;
}

return tokens;
}
