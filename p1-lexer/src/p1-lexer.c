/**
 * @file p1-lexer.c
 * @brief Compiler phase 1: lexer
 */
/* Use of Copilot to assist in regex creation and code autocomplete, 
and some clean up help and ChatGPT create tests. */
#include "p1-lexer.h"

void cleanup_and_exit(Regex *invalid_regexes[], size_t size) {
      for (int i = 0; i < size; i++) {
          Regex_free(invalid_regexes[i]);
      }
}

TokenQueue *
lex (const char *text)
{
  if (text == NULL)
    {
      Error_throw_printf ("Lexer received NULL input string");
    }
  TokenQueue *tokens = TokenQueue_new ();
  /* compile regular expressions */
  Regex *whitespace = Regex_new ("^[ \n\t\r]");
  Regex *comment = Regex_new ("^//[^\n\r]*");
  Regex *symbol = Regex_new ("^[][(){};=,+*-/%<>!]");
  Regex *double_symbol = Regex_new ("^(==|<=|>=|!=|&&|\\|\\|)");
  Regex *decimal_int = Regex_new ("^(0|[1-9][0-9]*)");
  Regex *identifier = Regex_new ("^([a-zA-Z][a-zA-Z0-9_]*)");
  Regex *string
      = Regex_new ("^\"([^\n\r\"\\\\]|(\\\\\\\\)|\\\\\"|\\\\n|\\\\t)*\"");
  Regex *hex_literal = Regex_new ("^(0x[0-9a-fA-F]+)");
  Regex *key_words
      = Regex_new ("^\\b(if|else|while|return|int|def|true|false|void)\\b");
  Regex *invalid_words
      = Regex_new ("^\\b(for|callout|class|interface|extends|implements|new|"
                   "this|string|float|double|null)\\b");

  Regex *invalid_regexes[] = {whitespace, comment, symbol, double_symbol, decimal_int, identifier, string, hex_literal, key_words, invalid_words};
  size_t regex_array_len = sizeof(invalid_regexes) / sizeof(invalid_regexes[0]);

  /* read and handle input */
  /* Read through decaf and understand program*/
  char match[MAX_TOKEN_LEN];
  int line_count = 1;

  while (*text != '\0')
    {
      /* match regular expressions */
      
      /* ignore whitespace or comments */
      if (Regex_match (whitespace, text, match)
          || Regex_match (comment, text, match))
        {
          /* increase line count */
          if (strncmp (match, "\n", MAX_TOKEN_LEN) == 0)
            {
              line_count = line_count + 1;
            }
        }
      else if (Regex_match (identifier, text, match))
        {
          /* check if identifier is a keyword */
          char keyword_match[MAX_TOKEN_LEN];
          if (Regex_match (key_words, text, keyword_match))
            {
              TokenQueue_add (tokens,
                              Token_new (KEY, keyword_match, line_count));
            }
          else
            {
              /* check if identifier is an illegal word */
              char invalid_match[MAX_TOKEN_LEN];
              if (Regex_match (invalid_words, text, invalid_match))
                {
                  cleanup_and_exit(invalid_regexes, regex_array_len);
                  Error_throw_printf (
                      "Reserved word: \"%s\"\n",
                      invalid_match);
                }
              else
                {
                  TokenQueue_add (tokens, Token_new (ID, match, line_count));
                }
            }
        }
      else if (Regex_match (double_symbol, text, match)
               || Regex_match (symbol, text, match)) /* first checks for double symbols, then single symbols */
        {
          TokenQueue_add (tokens, Token_new (SYM, match, line_count));
        }
      else if (Regex_match (hex_literal, text, match)
               || Regex_match (decimal_int, text, match)) /* first checks for hex literals, then decimal integers */
        {
          bool hex_match = Regex_match (hex_literal, text, match);
          TokenQueue_add (tokens, Token_new (hex_match ? HEXLIT : DECLIT,
                                             match, line_count));
        }
      else if (Regex_match (string, text, match)) /* matches strings */
        {
          TokenQueue_add (tokens, Token_new (STRLIT, match, line_count));
        }
      else
        {
          /* prints error line */
          /* prints where the lexer last left of, and ends at the end of the line */
          int error_text_length = strlen(text);
          char invalid_match[error_text_length + 1];
          for(int i = 0; i < error_text_length; i++) {
            if(text[i] == '\n' || text[i] == '\r' || text[i] == '\t' || text[i] == ' ') {
              invalid_match[i] = '\0';
              break;
            }
            invalid_match[i] = text[i];
          }
          cleanup_and_exit(invalid_regexes, regex_array_len);
          Error_throw_printf ("Invalid token on line %d: \"%s\"\n", line_count, invalid_match);
        }
      /* skip matched text to look for next token */
      text += strlen (match);
    }

  /* clean up */
  cleanup_and_exit(invalid_regexes, regex_array_len);

  return tokens;
}
