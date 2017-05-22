/*
 * getword.c *
 *
 *      Author: Artur Prusinowski
 *      Class: CS570
 *      Instructor: Dr. Carroll
 *      Assignment: Program 1, Due 02/10/2016
 *
 *      Implementation of the getword(char *) function. Returns size of the
 *      word or -1 if there is no more words in stdin.
 */
#include "getword.h"


extern int UM_QUOTE;
extern int ENV_VAR_VALUE;

int isCharMeta(int);
int isCharDelim(int);
int isEndofString(int);
int maxString(int*, char*, int*);

int nextChar(char*, int*, int*, int*);
int nextCharSizeZero(char*, int*, int*, int*);
int nextCharInQuotes(char*, int*, int*, int*);

int getword(char *w) {
    int iochar;
    int size = 0;
    int backSlashSet = 0;
    iochar = getchar();
    w[0] = '\0';
    if (iochar == '\'') {
        ENV_VAR_VALUE=0;
        iochar = getchar();
        return nextCharInQuotes(w, &iochar, &size, &backSlashSet);

    }
    return nextCharSizeZero(w, &iochar, &size, &backSlashSet);
}


/*
 * Function is called when the string processed so far is non-empty and the next
 * character is not a single quote.
 * PARAMETERS:
 * w            -- Pointer to char array for the currently constructed string
 * iochar       -- Pointer to int containing the next character to be processed
 * size         -- Pointer to int containing the current size of the char array
 * backSlashSet -- Pointer to int containing the value of the backslash flag
 *
 * RETURNS:
 * Function returns an int containing the size of the char array constructed.
 *
 */
int nextChar(char *w, int *iochar, int *size, int * backSlashSet) {
    
    //if((*iochar) == '$' && !ENV_VAR_VALUE)ENV_VAR_VALUE = 1;   
    
    if (*size == (STORAGE - 1)) return maxString(size, w, iochar);
    
    else if ((*iochar) == '\'' && !((*backSlashSet))) {
        *iochar = getchar();
        return nextCharInQuotes(w, iochar, size, backSlashSet);
        
    }
    else if (((isCharDelim((*iochar))) && !(*backSlashSet)) || isEndofString((*iochar))) {
        ungetc((*iochar), stdin);
        return (*size);
        
    }
    else if ((*iochar) == '\\' && !((*backSlashSet))) {
        (*backSlashSet) = 1;
        *iochar = getchar();
        return nextChar(w, iochar, size, backSlashSet);
        
    } else {
        w[(*size)] = *iochar;
        w[++(*size)] = '\0';
        (*backSlashSet) = 0;
        *iochar = getchar();
        return nextChar(w, iochar, size, backSlashSet);
        
    }

}
/*
 * Function is called when there has not been any characters stored in the char array.
 *
 * PARAMETERS:
 * w            -- Pointer to char array for the currently constructed string
 * iochar       -- Pointer to int containing the next character to be processed
 * size         -- Pointer to int containing the current size of the char array
 * backSlashSet -- Pointer to int containing the value of the backslash flag
 *
 * RETURNS:
 * Function returns an int containing the size of the char array constructed.
 *
 */
int nextCharSizeZero(char *w, int *iochar, int *size, int *backSlashSet) {

    if( (*iochar) == '$' && !(*backSlashSet)){ 
        ENV_VAR_VALUE = 1; 
        w[(*size)] = *iochar;
        w[++(*size)] = '\0';      
        *iochar = getchar();
        return nextCharSizeZero(w, iochar, size, backSlashSet);        
    }    
    
    if ((*iochar) == EOF) {
        backSlashSet = 0;
        return -1;
        
    }  
    else if ((*iochar) == '\n' || ((*iochar) == ';' && !(*backSlashSet))) {
        backSlashSet = 0;
        return 0;
        
    }
    else if ((*iochar) == '\\' && !((*backSlashSet) == 1)) {
        (*backSlashSet) = 1;
        (*iochar) = getchar();
        return nextCharSizeZero(w, iochar, size, backSlashSet);
        
    } else if ((*iochar) == '\'' && !((*backSlashSet))) {       
        
        (*iochar) = getchar();
        return nextCharInQuotes(w, iochar, size, backSlashSet);
        
    }
    else if ((*iochar) == ' ' && !(*backSlashSet)) {        
        ENV_VAR_VALUE = 0;
        (*iochar) = getchar();
        return nextCharSizeZero(w, iochar, size, backSlashSet);
        
    } else if ((isCharMeta((*iochar)) && !(*backSlashSet))) {
        w[(*size)++] = *iochar;
        w[(*size)] = '\0';
        return 1;
        
    } else {
        w[(*size)] = *iochar;
        w[++(*size)] = '\0';
        (*backSlashSet) = 0;
        *iochar = getchar();
        return nextChar(w, iochar, size, backSlashSet);
    }
}
/*
 * Function is called when the next character to be processed is a single quote.
 *
 * PARAMETERS:
 * w            -- Pointer to char array for the currently constructed string
 * iochar       -- Pointer to int containing the next character to be processed
 * size         -- Pointer to int containing the current size of the char array
 * backSlashSet -- Pointer to int containing the value of the backslash flag
 *
 * RETURNS:
 * Function returns an int containing the size of the char array constructed.
 *
 */
int nextCharInQuotes(char *w, int *iochar, int *size, int *backSlashSet) {
    UM_QUOTE = 1;   
      
    
    if (*size == (STORAGE - 1)) return maxString(size, w, iochar);    
    

    else if (isEndofString(*iochar) || ((*iochar) == ';' && !(*backSlashSet))) {
        backSlashSet = 0;
        w[*size] = '\0';
        ungetc((*iochar), stdin);
        return *size;
        
    } else if ((*iochar) == '\\' && !((*backSlashSet) == 1)) {
        (*backSlashSet) = 1;
        w[(*size)++] = *iochar;
        (*iochar) = getchar();
        return nextCharInQuotes(w, iochar, size, backSlashSet);
        
    } else if ((*iochar) == '\'' && !(*backSlashSet)) {
        if(UM_QUOTE) UM_QUOTE = 0;
        *iochar = getchar();
        return nextChar(w, iochar, size, backSlashSet);
        
    } else if ((*iochar) == '\'' && (*backSlashSet)) {
        
        (*backSlashSet) = 0;
        w[(*size) - 1] = *iochar;
        (*iochar) = getchar();
        return nextCharInQuotes(w, iochar, size, backSlashSet);
        
    }
    else {
        w[(*size)] = *iochar;
        w[++(*size)] = '\0';
        (*backSlashSet) = 0;
        *iochar = getchar();
        return nextCharInQuotes(w, iochar, size, backSlashSet);
    }
}
/*
/**********************Helper Functions********************************
 */
/*
int isCharMeta(int iochar) {
    return (iochar == '\<' ||
            iochar == '\>' ||
            iochar == '\|' ||
            iochar == '\;' ||
            iochar == '\$' ||
            iochar == '\&');
}
int isCharDelim(int iochar) {
    return (iochar == '\<' ||
            iochar == '\>' ||
            iochar == '\|' ||
            iochar == '\;' ||
            iochar == ' ' ||
            iochar == '\&');
}
 
 
 */



int isCharMeta(int iochar) {
    return (iochar == '<' ||
            iochar == '>' ||
            iochar == '|' ||
            iochar == ';' ||
            iochar == '$' ||
            iochar == '&');
}




int isCharDelim(int iochar) {
    return (iochar == '<' ||
            iochar == '>' ||
            iochar == '|' ||
            iochar == ';' ||
            iochar == ' ' ||
            iochar == '&');
}

int isEndofString(int iochar) {
    return (iochar == '\n' ||
            iochar == EOF);
}

int maxString(int * size, char * w, int *iochar) {
    if (*size == (STORAGE - 1)) {
        ungetc(*iochar, stdin);
        w[STORAGE] = '\0';
        return *size;
    }
    return 0;
}
