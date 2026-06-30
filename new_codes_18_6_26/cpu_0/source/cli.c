/***************************************************************************//**
 * @file     cli.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Jan 09, 2026
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    SG
 * @par Functions Included
 * void cliUART(void); <br>
 * void cli_processCommand(char* str); <br>
 * uint32_t toint(char* str); <br>
 * uint32_t hex2int(char *hex); <br>
 * char* my_strtok(char* str, char delim);
 ******************************************************************************/

#include "cmds.h"
#include "ctype.h"
#include "common_header.h"
#include "fifo.h"
#include "app.h"
#include "bsp.h"

#define MODE_LOCAL 1
#define CLI_LOCAL 0

volatile uint8_t isCLIBusy = 0;
char* token;
uint8_t mode = MODE_LOCAL;
uint8_t ERROR_CODE = PASS;
uint8_t data[16];
extern uint8_t volatile count;
char cmd_local[80];

category_t *categoryLocal, *category;
char* modeList[] = {"local"};
category_t categoryList[] = {{"top",0,CAT_APP}
                            };

#define CLI_UART_PROMPT {\
        printf("\r\n%s>", "CCC_CPU0");\
    if(strcmp(category->name,"top")) {\
        printf("%s> ",category->name);\
      }\
}

/**
 * @brief Handles CLI input over UART interface.
 *
 * Reads characters from the CLI UART RX FIFO, processes user input,
 * supports basic line editing (backspace/delete), echoes characters,
 * and triggers command execution upon receiving end-of-line (EOL/CR).
 * It also manages CLI prompt display and prevents buffer overflow.
 *
 * @param [in,out] None
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note 
 * - Uses a static buffer (cmd_local) to accumulate user input.
 * - Input is converted to lowercase before processing.
 * - Supports backspace/delete for editing input line.
 * - Prevents buffer overflow by limiting input length.
 * - Command execution is handled via cli_processCommand().
 * - Ensures mutual exclusion using isCLIBusy flag.
 * - Handles CR/LF combinations carefully to avoid duplicate prompts.
 */
 
void cliUART(void) {
    static uint32_t d;
    uint8_t ch;
    static uint8_t k_local = 0U;
    static uint8_t first = 1U;
    static uint8_t lastchar = CR;

    if(first) {
        categoryLocal = &categoryList[0];
        category =  categoryLocal;
        CLI_UART_PROMPT
        first = 0;
    }
		if (fifo_read(uart_cli.prx,&ch) != 0U) {
			return;
		}

		if(ch == 0x00U) 
		{
			return;
		}

		if (ch == BS || ch == DEL) {
	   if(k_local!=0U) {
	     k_local--;
	     printf("\b \b");
	   }
	 }
	 else {
	   ch = tolower(ch);
	   cmd_local[k_local++] = ch;
	  if(ch !='\n')
		{
	      printf("%c",ch);
		}
	  else
		{
				printf(""); // on EOL donot echo as it add additional line between prompts
		}
	 }

	   if (ch == EOL || ch == CR || k_local > 75U) { // check add to avoid cmd_local buffer overflow
	   	if (k_local > 0U) {
	   		if(cmd_local[0] == EOL || cmd_local[0] == CR) {
			    // on receiving CRLF 2 prompts are printed 1 for CR and 1 for LF
                // So if CR received before LF then prompt on LF isn't printed
             if(cmd_local[0] == EOL && lastchar == CR){	
			 	// do nothing
			 }
             else
						 {
               CLI_UART_PROMPT
						 }
            }
            else {
                cmd_local[k_local-1U] = EOS;
                while(isCLIBusy);
                isCLIBusy = 1;
                cli_processCommand(cmd_local);
                CLI_UART_PROMPT
                isCLIBusy = 0;
            }
        }
        k_local = 0;
        lastchar = ch;
       }
}

/**
 * @brief Processes CLI command string and executes corresponding action.
 *
 * Parses the input command string, identifies whether it is a help request,
 * category change, or a valid command, and executes the associated command
 * function. It also handles command validation, error reporting, and response
 * formatting.
 *
 * @param [in,out] str Pointer to input command string to be parsed and processed.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Uses tokenization (my_strtok) to parse the command string.
 * - Supports "help" command to list available commands in the current category.
 * - Allows switching between command categories.
 * - Executes matched command via function pointer (cmdFunc).
 * - Provides standardized responses: "#OK#", "#FAIL#", and format errors.
 * - Uses global ERROR_CODE to determine execution result.
 * - Restores original category context after command execution.
 */

void cli_processCommand(char* str) {
    uint8_t k =0;
    uint8_t categoryListDepth = sizeof(categoryList)/sizeof(category_t);
    category_t* cat = categoryLocal;

    token = my_strtok(str,DELIM);

    if (!strcmp(token, "help")) {
        //CLI_PRINTF("\r\n\r\nCATEGORIES:");
        for (k=0; k<categoryListDepth;k++) {
            //CLI_PRINTF(" %s",categoryList[k].name);
        }
        printf("\r\n\r\n");

        for (k=0; k<cmdListDepth;k++) {
            if (cmdList[k].category == category->id && !cmdList[k].hidden)
                printf("  %-20s %-40s %s\r\n",cmdList[k].cmdString,cmdList[k].formatString,cmdList[k].helpString);
        }
        return;
    }

    while (strcmp(token, categoryList[k].name)) {
        k++;
        if (k == categoryListDepth) {
			break;
		}
    }

    if (k <= categoryListDepth - 1U) { // category change request
        categoryLocal = &categoryList[k];
        category = &categoryList[k];
      return;
    }

    k = 0;

    while (!(cmdList[k].category == cat->id && !strcmp(token, cmdList[k].cmdString) && token[0] != NULL)) {
        if (k == cmdListDepth - 1U) {
            printf("\r\n\r\nNot valid command !\r\n");
						printf("#FAIL#\r\n");
            return;
        }
        k++;
    }
		
    printf("\r\n\r\n"); // insert newlines before results are printed
    category = cat;
    cmdList[k].cmdFunc();
    category = categoryLocal;
    printf("\r\n"); // insert newlines after results are printed
    if (ERROR_CODE == FAIL) {
        //CLI_PRINTF("#FAIL#\r\n\b"); //adding BS for detecting end of response sent to remote side
        printf("#FAIL#\r\n"); //adding BS for detecting end of response sent to remote side
    }
    else if (ERROR_CODE == WRONG_FORMAT) {
        printf("Wrong Format\r\n%s\r\n",cmdList[k].helpString);
        printf("#FAIL#\r\n");
    }
    else if (ERROR_CODE == PASS) {
        //CLI_PRINTF("#OK#\r\n\b");
        printf("#OK#\r\n");
    }
		else {
					// Do Nothing
		}
    ERROR_CODE = PASS; // clear error
}

/**
 * @brief Converts a string to an integer (decimal or hexadecimal).
 *
 * Parses the input string and converts it into a 32-bit integer value.
 * The function supports both decimal and hexadecimal formats. Hexadecimal
 * values must be prefixed with "0x". Leading spaces are ignored.
 *
 * @param [in] str Pointer to null-terminated string to be converted.
 *
 * @return uint32_t Converted integer value.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - If the string starts with "0x", it is treated as hexadecimal and
 *   converted using hex2int().
 * - Otherwise, it is treated as decimal and converted using atoi().
 * - Leading spaces are skipped during parsing.
 * - Returns 0 if the string is empty or invalid.
 */

uint32_t toint(char* str) {
    uint8_t hex = 0;
    while (*str != EOS) {
        if(*str == SPACE) {
					str++; 
					continue;
				}
        if(*str == '0' && *(str+1) == 'x') {
					str+=2; 
					hex = 1; 
					continue;
				}
        if (!hex) {
					return(atoi(str));
				}
				else {
					return(hex2int(str));
				}
    }
    return 0;
}

/**
 * @brief Converts a hexadecimal string to a 32-bit integer.
 *
 * Parses a null-terminated string representing a hexadecimal number
 * and converts it into its corresponding 32-bit integer value.
 * Each character is processed and mapped to its 4-bit equivalent.
 *
 * @param [in] hex Pointer to null-terminated hexadecimal string.
 *
 * @return uint32_t Converted integer value.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Supports both uppercase ('A'-'F') and lowercase ('a'-'f') hex digits.
 * - Assumes input string contains only valid hexadecimal characters.
 * - Does not handle "0x" prefix explicitly; it should be removed beforehand if present.
 * - No error checking is performed for invalid characters.
 */
uint32_t hex2int(char *hex) {
    uint32_t val = 0;
    while (*hex) {
        // get current character then increment
        uint8_t byte = *hex++;
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9') {
					byte = byte - '0';
				}
        else if (byte >= 'a' && byte <='f') {
					byte = byte - 'a' + 10;
				}
        else if (byte >= 'A' && byte <='F') {
					byte = byte - 'A' + 10;
				}
				else{
					//do nothing
				}
        // shift 4 to make space for new digit, and add the 4 bits of the new digit
        val = (val << 4U) | (byte & 0xFU);
    }
    return val;
}

/**
 * @brief Tokenizes a string based on a delimiter (custom strtok implementation).
 *
 * Splits the input string into tokens using the specified delimiter.
 * Maintains internal static state to allow successive calls for parsing
 * the same string, similar to standard strtok().
 *
 * @param [in,out] str Pointer to input string to tokenize. Pass NULL for
 *                     subsequent calls to continue tokenizing the same string.
 * @param [in] delim Delimiter character used to split the string.
 *
 * @return char* Pointer to the next token in the string.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Modifies the input string by replacing delimiter or line-ending characters
 *   (EOL/CR) with null terminator ('\0').
 * - Uses static internal pointer, hence not re-entrant or thread-safe.
 * - Tokenization stops at delimiter, end-of-string (EOS), EOL, or CR.
 */

char* my_strtok(char* str, char delim) {
    static char* s;
    char* r;

    if (str != NULL) {
			s = str;
		}
    r = s;

    while (!(*s == EOS || *s == delim || *s == EOL || *s == CR))  { // Exit on NULL or delimiter
        s++;
    }
    if (*s == delim) { 
			*s = '\0'; 
			s++;
		}
    else {
			*s = '\0';
		}

    return r;
}
