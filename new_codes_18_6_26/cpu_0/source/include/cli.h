/***************************************************************************//**
 * @file     cli.h
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Dec 24, 2025
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    SG 
 ******************************************************************************/

#ifndef CLI_H__
#define CLI_H__

#include <stdint.h>

#define DELIM ' '
#define CAT_APP    0 
#define CAT_LIU    1

#define EOL '\n'
#define CR  '\r'
#define EOS '\0'
#define UP     0x08
#define DOWN   0x08
#define BS     0x08
#define DEL    0x7F
#define SPACE ' '
#define COMMA ','

#define PASS         0U
#define FAIL         1U
#define WRONG_FORMAT 2U

//macro
#define CHECK_ERROR(cond, dothis) if(*token == EOS) {\
	                        if(cond) dothis\
                            else {ERROR_CODE = WRONG_FORMAT; return;}\
	                      }
// command format struct
typedef struct {
  uint8_t category;
	char    helpString[40];
	char    formatString[40];
	char    cmdString[30];
  void    (*cmdFunc)(void);
  char    hidden;
} cmdFormat_t;

typedef struct {
	char name[8]; // name to be displayed
  uint8_t instance; // instance number - 1/2/3
  uint8_t id;   // id in command list 
} category_t;

//variables
extern category_t* category;
extern char* token;
extern uint8_t ERROR_CODE;

//functions
void cli(uint8_t active_link_nok);
void cliUART();
void cli_processCommand(char* str);
void cli_spiwr(void);
void cli_spird(void);

uint32_t toint(char* str);
uint32_t hex2int(char *hex);
char* my_strtok(char* str, char delim);
void outbyte (char ch);
#endif
