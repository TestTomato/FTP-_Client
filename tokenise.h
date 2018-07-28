 /*
  * Author: Au Yong Tuck Loen , Bryan Leong Yi Jun
  * Date: 29/07/18
  * Filename: tokenise.h
  * Description: Prototype function for splitting string with whitespace characters.
  */

   
   #define MAX_NUM_TOKENS  100
   #define tokenSeparators " \t\n"    // characters that separate tokens
  
  /*
   * breaks up chars array with whitespace characters into seperate individual tokens.
   * return value greater than or equal to 0 whcih represents largest index of token array
   * ,otherwise -1 on failure
   */
  int tokenise (char line[], char *token[]);
 
