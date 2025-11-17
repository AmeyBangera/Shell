#include "parser.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

// ############## LLM Generated Code Begins ##############

bool parse_shell_cmd(const char** input);
bool parse_cmd_group(const char** input);
bool parse_atomic(const char** input);
bool parse_input_redirect(const char** input);
bool parse_output_redirect(const char** input);
bool parse_name(const char** input);
void skip_whitespace(const char** input);

// Main parse function
bool parse_input(const char* input) {
    const char* current = input;
    skip_whitespace(&current);
    
    // Empty input is valid
    if (*current == '\0') {
        return true;
    }
    
    bool result = parse_shell_cmd(&current);
    skip_whitespace(&current);
    
    // Make sure we consumed all input
    return result && *current == '\0';
}

// Parse shell_cmd -> cmd_group ((& | ;) cmd_group)* &?
bool parse_shell_cmd(const char** input) {
    if (!parse_cmd_group(input)) {
        return false;
    }
    
    skip_whitespace(input);
    
    while (**input == '&' || **input == ';') {
        char separator = **input;
        (*input)++;
        skip_whitespace(input);
        if(separator == '&' && **input == '\0'){
            return true;
        }
    
        if (!parse_cmd_group(input)) {
            return false;
        }
        
        skip_whitespace(input);
    }
    
    // Optional trailing &
    // if (**input == '&') {
    //     (*input)++;
    //     skip_whitespace(input);
    // }
    
    return true;
}

// Parse cmd_group -> atomic (\| atomic)*
bool parse_cmd_group(const char** input) {
    if (!parse_atomic(input)) {
        return false;
    }
    
    skip_whitespace(input);
    
    while (**input == '|') {
        (*input)++;  // Consume |
        skip_whitespace(input);
        
        if (!parse_atomic(input)) {
            return false;
        }
        
        skip_whitespace(input);
    }
    
    return true;
}

// Parse atomic -> name (name | input | output)*
bool parse_atomic(const char** input) {
    if (!parse_name(input)) {
        return false;
    }
    
    skip_whitespace(input);
    
    while (**input != '\0' && **input != '|' && **input != '&' && **input != ';') {
        const char* old_pos = *input;
        
        if (parse_name(input) || parse_input_redirect(input) || parse_output_redirect(input)) {
            skip_whitespace(input);
        } else {
            *input = old_pos;
            break;
        }
    }
    
    return true;
}

// Parse input -> < name | <name
bool parse_input_redirect(const char** input) {
    if (**input != '<') {
        return false;
    }
    
    (*input)++;  // Consume <
    skip_whitespace(input);
    
    return parse_name(input);
}

// Parse output -> > name | >name | >> name | >>name
bool parse_output_redirect(const char** input) {
    if (**input != '>') {
        return false;
    }
    
    (*input)++;  // Consume first >
    
    if (**input == '>') {
        (*input)++;  // Consume second > if present
    }
    
    skip_whitespace(input);
    
    return parse_name(input);
}

// Parse name -> r"[^|&><;]+"
bool parse_name(const char** input) {
    if (**input == '\0' || **input == '|' || **input == '&' || **input == '>' ||
        **input == '<' || **input == ';') {
            return false;
        }
        
        // Consume all characters that are not special
        while (**input != '\0' && **input != '|' && **input != '&' && **input != '>' &&
            **input != '<' && **input != ';' && !isspace(**input)) {
                (*input)++;
            }
            
            return true;
        }
        
        // Helper function to skip whitespace
        void skip_whitespace(const char** input) {
            while (**input != '\0' && isspace(**input)) {
                (*input)++;
            }
        }
// ############## LLM Generated Code Ends ################