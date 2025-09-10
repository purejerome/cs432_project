/**
 * @file p1-lexer.c
 * @brief Compiler phase 1: lexer
 */
// Use of Copilot to assist in regex creation and ChatGPT create tests.
#include "p1-lexer.h"

TokenQueue *
lex (const char *text)
{
  TokenQueue *tokens = TokenQueue_new ();
  /* compile regular expressions */
  Regex *whitespace = Regex_new ("^[ \n\t\r]");
  Regex *comment = Regex_new ("^//[^\n\r]*");
  Regex *symbol = Regex_new ("^[][(){};=,+*-/%<>!]");
  Regex *double_symbol = Regex_new ("^(==|<=\\(?!=\\)|>=\\(?!=\\)|!=\\(?!=\\)|&&|\\|\\|)");
  Regex *decimal_int = Regex_new ("^(0|[1-9][0-9]*)");
  Regex *identifier = Regex_new ("^([a-z A-Z][a-zA-Z0-9_]*)");
  Regex *string
      = Regex_new ("^\"([^\n\r\"\\\\]|(\\\\\\\\)|\\\\\"|\\\\n|\\\\t)*\"");
  Regex *hex_literal = Regex_new ("^(0x[0-9a-fA-F]+)");
  Regex *key_words
      = Regex_new ("^\\b(if|else|while|return|int|def|true|false)\\b");
  Regex *invalid_words
      = Regex_new ("^\\b(for|callout|class|interface|extends|implements|new|"
                   "this|string|float|double|null)\\b");

  /* read and handle input */
  /* Read through decaf and understand program*/
  char match[MAX_TOKEN_LEN];
  int line_count = 1;
  if (text == NULL)
    {
      Error_throw_printf ("lexer received NULL input string");
    }

  while (*text != '\0')
    {
      /* match regular expressions */
      if (Regex_match (whitespace, text, match)
          || Regex_match (comment, text, match))
        {
          /* ignore whitespace */
          if (strncmp (match, "\n", MAX_TOKEN_LEN) == 0)
            {
              line_count = line_count + 1;
            }
        }
      else if (Regex_match (identifier, text, match))
        {
          char keyword_match[MAX_TOKEN_LEN];
          if (Regex_match (key_words, text, keyword_match))
            {
              TokenQueue_add (tokens,
                              Token_new (KEY, keyword_match, line_count));
            }
          else
            {
              char invalid_match[MAX_TOKEN_LEN];
              if (Regex_match (invalid_words, text, invalid_match))
                {
                  Error_throw_printf (
                      "reserved word used as identifier on line %d: %s\n",
                      line_count, invalid_match);
                }
              else
                {
                  TokenQueue_add (tokens, Token_new (ID, match, line_count));
                }
            }
        }
      else if (Regex_match (double_symbol, text, match)
               || Regex_match (symbol, text, match))
        {
          TokenQueue_add (tokens, Token_new (SYM, match, line_count));
        }
      else if (Regex_match (hex_literal, text, match)
               || Regex_match (decimal_int, text, match))
        {
          bool hex_match = Regex_match (hex_literal, text, match);
          TokenQueue_add (tokens, Token_new (hex_match ? HEXLIT : DECLIT,
                                             match, line_count));
        }
      else if (Regex_match (string, text, match))
        {
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
  Regex_free (string);
  Regex_free (hex_literal);
  Regex_free (key_words);
  Regex_free (double_symbol);
  Regex_free (invalid_words);
  Regex_free (comment);

  return tokens;
}
