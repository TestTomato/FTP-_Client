 /*
  * Author: Au Yong Tuck Loen , Bryan Leong Yi Jun
  * Date: 29/07/18
  * Filename: tokenise.c
  * Description: Defining function for splitting string with whitespace characters.
  */
  
   #include <string.h>
   #include "tokenise.h"
  
  int tokenise (char line[], char *token[])
  {
        char *tk;
        int n = 0;
  
        tk = strtok(line, tokenSeparators);
        token[n] = tk;
  
        while (tk != NULL) {
 
            ++n;

            if (n >=MAX_NUM_TOKENS) {
                n = -1;
                break;
            }
  
            tk = strtok(NULL, tokenSeparators);
            token[n] = tk;
        }

        return n;
  }
