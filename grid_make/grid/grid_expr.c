/******************************************************************************

                            Online C Compiler.
                Code, Compile, Run and Debug C program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/


#include "grid_expr.h"

void grid_expr_init(struct grid_expr_model* expr){

    expr->current_event = NULL;

    grid_expr_clear_input(expr);
    grid_expr_clear_output(expr);

}


grid_expr_clear_input(struct grid_expr_model* expr){

    expr->input_string_length = 0;

    for (uint32_t i=0; i<GRID_EXPR_INPUT_STRING_MAXLENGTH; i++){

        expr->input_string[i] = 0;

    }

}

grid_expr_clear_output(struct grid_expr_model* expr){


    expr->output_string_length = 0;

    for (uint32_t i=0; i<GRID_EXPR_OUTPUT_STRING_MAXLENGTH; i++){

        expr->output_string[i] = 0;

    }

}



grid_expr_set_current_event(struct grid_expr_model* expr, struct grid_ui_event* eve){

    expr->current_event = eve;
}


grid_expr_evaluate(struct grid_expr_model* expr, char* input_str, uint8_t input_length){

    grid_expr_clear_input(expr);
    grid_expr_clear_output(expr);

    for (uint32_t i=0; i<input_length; i++){

        expr->input_string[i] = input_str[i];
        
    }

    expr->input_string_length = input_length;



    subst_all_variables_starting_from_the_back(expr->input_string, expr->input_string_length);    
    subst_all_functions_starting_from_the_back(expr->input_string, expr->input_string_length);

    int32_t result = expression(expr->input_string);


    printf("Result: %d\r\n", result);

    printf("Result String: \"%s\"\r\n", &expr->output_string[GRID_EXPR_OUTPUT_STRING_MAXLENGTH-expr->output_string_length]);

}
  
  
int* e_param_list = 0;
int  e_param_list_length = 0;  


int* p_param_list = 0;
int  p_param_list_length = 0;  


//https://stackoverflow.com/questions/9329406/evaluating-arithmetic-expressions-from-string-in-c
char peek(char** e)
{
    return **e;
}

char peek2(char** e)
{
    return *((*e)+1);
}


char get(char** e)
{
    char ret = **e;
    ++*e;
    return ret;
}

int number(char** e)
{
    int result = get(e) - '0';
    while (peek(e) >= '0' && peek(e) <= '9') // HEX para
    {
        result = 10*result + get(e) - '0'; // HEX para
    }
    return result;
}

int expr_level_3(char** e) // factor
{
    if (peek(e) >= '0' && peek(e) <= '9') // HEX para
        return number(e);
    else if (peek(e) == '(')
    {
        get(e); // '('
        int result = expr_level_0(e);
        get(e); // ')'
        return result;
    }
    else if (peek(e) == '-')
    {
        get(e);
        return -expr_level_3(e);
    }
    printf("ERROR in expr_level_3()\n");
    return 0; // error
}

int expr_level_2(char ** e) // term
{
    int result = expr_level_3(e);
    while (peek(e) == '*' || peek(e) == '/' || peek(e) == '%'){
    
        char peeked = get(e);
        
        if (peeked == '*'){
            result *= expr_level_3(e);
        }
        else if (peeked == '%'){
            result %= expr_level_3(e);
        }
        else{
            result /= expr_level_3(e);
            
        }
    }
    return result;
}

int expr_level_1(char ** e) // equality
{
    int result = expr_level_2(e);
    while (peek(e) == '+' || peek(e) == '-')
        if (get(e) == '+')
            result += expr_level_2(e);
        else
            result -= expr_level_2(e);
    return result;
}

int expr_level_0(char ** e) // equality
{
    int result = expr_level_1(e);
    
    
    while (     (peek(e) == '>' && peek2(e) != '=') || 
                (peek(e) == '<' && peek2(e) != '=') || 
                (peek(e) == '=' && peek2(e) == '=') ||
                (peek(e) == '!' && peek2(e) == '=') ||
                (peek(e) == '>' && peek2(e) == '=') ||
                (peek(e) == '<' && peek2(e) == '=') 
            ){
        
        char peeked = get(e);
        char peeked2 = peek(e);

        if ((peeked == '>' && peeked2 != '=')){
            result = (result>expr_level_1(e));
        }
        else if (peeked == '<' && peeked2 != '='){
            result = (result<expr_level_1(e));
        }
        else if (peeked == '=' && peeked2 == '='){
            get(e); // burn the second character
            result = (result == expr_level_1(e));
        }
        else if (peeked == '!' && peeked2 == '='){
            get(e); // burn the second character
            result = (result != expr_level_1(e));
        }
        else if (peeked == '>' && peeked2 == '='){
            get(e); // burn the second character
            result = (result >= expr_level_1(e));
        }
        else if (peeked == '<' && peeked2 == '='){
            get(e); // burn the second character
            result = (result <= expr_level_1(e));
        }
    }
    return result;
}

int expression_inner(char ** e)
{
    int result = expr_level_0(e);

    return result;
}

int expression(char * str){


    return expression_inner(&str);
}

void insertTo(char* start,int length,char* that){
    
    char ending[100] = {0};
    
    //printf("insertTo: Hova: %s Milyen hosszú helyre: %d Mit: %s\n", start, length, that);
    
    sprintf(ending,"%s",start+length);
    sprintf(start,"%s",that);
    sprintf(start+strlen(that),"%s",ending);
}

int brack_len(char* funcDesc,int maxLen){ //pl.: almafa(6*(2+2))*45
    
        // START: SUKU
    
    int nyitCount = 0;
    int zarCount = 0;
    
    for(int i=0; i<maxLen; i++){
        
        if (funcDesc[i] == '('){
            
            nyitCount++;
        }
        else if (funcDesc[i] == ')'){
            zarCount++;
            
            if (zarCount == nyitCount){
                return i+1;
            }
        }
        
    }
}



void calcSubFnc(char* startposition){
    char* fName = startposition;
    char* fNameEnd = strstr(fName,"(");
    
    int max_offset = brack_len(fNameEnd,strlen(fNameEnd)) -2;
    
    printf("Maxoffset: %d  ## \r\n", max_offset);
    
    int param_expr_results[10] = {0};

    int param_expr_results_count = 0;
    
    char* start = fNameEnd+1;
 

    
    char* comma = strstr(start, ",");
    int commaoffset = -1;
    
    
    
    for (int i=0; i<max_offset; i=i){
        
        
        int commaoffset = -1;
        
        for(int j=i; j<max_offset; j++){
            
            if (start[j] == ','){
                commaoffset = j;
                break;
            }
        }
        
 
        if (commaoffset==-1){
            
            printf("No more commas! \r\n");
            
            char param_expr[20] = {0};
            
            for (int j=0; j<(max_offset-i); j++){
                param_expr[j] = start[i+j];
                
                
            }
            
            printf("Parameter: \"%s\", ", param_expr);
        
            
            param_expr_results[param_expr_results_count] = expression(param_expr);
            
            
            printf("Result: \"%d\" \r\n", param_expr_results[param_expr_results_count]);
            param_expr_results_count++;
            
            
            
            break;
        }
        else{
            printf("Commaoffset : %d: %d!  ", i, commaoffset);
            
            char param_expr[20] = {0};
            
            for (int j=0; j<commaoffset-i; j++){
                param_expr[j] = start[i+j];
                
            }
            
            printf("Parameter: \"%s\" , ", param_expr);
       
            
            param_expr_results[param_expr_results_count] = expression(param_expr);
            
            
            printf("Result: \"%d\" \r\n", param_expr_results[param_expr_results_count]);
            param_expr_results_count++;
            
            i=commaoffset+1;
            
        }
    }
    
    
    
    int resultOfFnc = 0;
    
    
    // START: CALC BUILTIN


    char justName[10] = {0};
    
    for (int i=0; i<9; i++){
        
        if (fName[i] == '('){
            break;
        }
        else{
            justName[i] = fName[i];
        }
        
    }

    
    if(strcmp(justName,"abs")==0){
        resultOfFnc = abs(param_expr_results[0]);
    }
    else if(strcmp(justName,"six")==0){
        resultOfFnc = 666666;
    }
    else if(strcmp(justName,"add")==0){
        resultOfFnc = param_expr_results[0] + param_expr_results[1];
    }
    else if(strcmp(justName,"print")==0){

        char fmt_str[] = "%02x";
        
        //printf("print_length: %d (%c)", param_expr_results[1], param_expr_results[1]+'0');

        if (param_expr_results[1]<=8){

            fmt_str[2] = param_expr_results[1]+'0';
        }
        else{

            fmt_str[2] = 8+'0';

        }
        
        uint8_t temp_array[20] = {0};
        uint8_t temp_array_length = 0;


        sprintf(temp_array, fmt_str, param_expr_results[0]);
        temp_array_length = strlen(temp_array);
        
        struct grid_expr_model* expr = &grid_expr_state;

        for (uint8_t i=0; i<temp_array_length; i++){

            expr->output_string[GRID_EXPR_OUTPUT_STRING_MAXLENGTH-expr->output_string_length-temp_array_length+i] = temp_array[i];

        }

        expr->output_string_length += temp_array_length;


        resultOfFnc = param_expr_results[0];
    }
    else if(strcmp(justName,"if")==0){
        
        if (param_expr_results[0]){
            resultOfFnc = param_expr_results[1];
        }else{
            resultOfFnc = param_expr_results[2];
        }
        
    }
    else{
        printf("Function \"%s\" not found!\n", justName);
        resultOfFnc = 0;
        
    }    
    
    // END;
    
    
    
    //printf("resultOfFnc: %d\n", resultOfFnc);
    

    
    char buff[100] = {0};
    
    sprintf(buff,"(%d)",resultOfFnc); //HEX para
    
    // hova, milyen hosszan, mit
    insertTo(startposition,(fNameEnd-fName)+max_offset+2,buff);
    
        
    //printf(" @@ debug: %s @@\n", startposition);
}

void subst_all_variables_starting_from_the_back(char* expr_string, int len){
    
    
    int izgi = 0;
    int var_end_pos = -1;
    char var_name[10] = {0};
    
    //printf("Subst Vars\n");
    
    // i must be signed int
    for(int i= len; i>=0; i--){
        
        //printf("i=%d\n",i);
        
        if (izgi == -1){
             
            // HEX para
            if ((expr_string[i] >= '0' && expr_string[i] <= '9') || 
                (expr_string[i] >= 'a' && expr_string[i] <= 'z') || 
                (expr_string[i] >= 'A' && expr_string[i] <= 'Z') || 
                (expr_string[i] == '_')){
                //továbbra is para van mert ez funtion
            }
            else {
                izgi=0;
                //printf("izgi=%d, i=%d\n", izgi, i);
            }
            
            
        }
        
        if (izgi == 0){
            // HEX para
            if  ((expr_string[i] >= '0' && expr_string[i] <= '9') || 
                (expr_string[i] >= 'a' && expr_string[i] <= 'z') || 
                (expr_string[i] >= 'A' && expr_string[i] <= 'Z') || 
                (expr_string[i] == '_')){
                
                if (expr_string[i+1] == '('){
                    
                    izgi = -1;
                    
                    //printf("izgi=%d, i=%d\n", izgi, i);
                    
                    
                }
                else{
                    
                    if ((expr_string[i] >= '0' && expr_string[i] <= '9')){
                        
                        izgi = 1;
                        //printf("izgi=%d, i=%d\n", izgi, i);
                        var_end_pos = i;
                        var_name[var_end_pos-i] = expr_string[i];                     
                        
                    }
                    else{
                        // nem csak szám van benne szóval fasz
                        izgi = 2;
                        //printf("izgi=%d, i=%d\n", izgi, i);
                        var_end_pos = i;
                        var_name[var_end_pos-i] = expr_string[i];   
                        
                    }                  
 
                    
                }
                
            }
            
        }
        else if (izgi == 1 || izgi == 2){
            // HEX para
            if (((expr_string[i] >= '0' && expr_string[i] <= '9') || 
            (expr_string[i] >= 'a' && expr_string[i] <= 'z') || 
            (expr_string[i] >= 'A' && expr_string[i] <= 'Z') || 
            (expr_string[i] == '_') )&& i!=0){
                
                
                var_name[var_end_pos-i] = expr_string[i];
                
                if ((expr_string[i] >= '0' && expr_string[i] <= '9')){
                    
                }
                else{
                    // nem csak szám van benne szóval fasz
                    izgi = 2;
                    
                }

                
            }
            else if (izgi==2){
                
                if (((expr_string[i] >= '0' && expr_string[i] <= '9') || 
                    (expr_string[i] >= 'a' && expr_string[i] <= 'z') || 
                    (expr_string[i] >= 'A' && expr_string[i] <= 'Z') || 
                    (expr_string[i] == '_') )&& i==0){
                        //printf("Special\n");
                        
                        var_name[var_end_pos-i] = expr_string[i];
                       i--;
                    }

                int var_name_len = strlen(var_name);
                
                // need to reverse the variable name string
                char var_name_good[10] = {0};
    
                for (int j = 0; j<var_name_len; j++){
                    var_name_good[j] = var_name[var_name_len-1-j];
                    var_name_good[j+1] = 0;
                }
                
                //printf("Variable \"%s\" found!\n", var_name_good);
                
                // TODO: Find variable registered in a list or something. Now var is always 1
                int variable_value = 1;
                
                if (var_name_len == 2){
                    if (var_name_good[0] == 'T'){
                        
                        if (var_name_good[1] >= '0' && var_name_good[1] <= '9' ){ //HEX para
                            
                            uint8_t index = var_name_good[1] - '0';
                            
                            variable_value = (int) grid_expr_state.current_event->parent->template_parameter_list[index];
                            
                        }
                        
                    }
                }
                
                
                char* found = &expr_string[i+1]; // i+1 helyen lesz mindenképpen!
                
                char buff[100] = {0};
                
                sprintf(buff,"%d",variable_value); // HEX para
                
                // hova, milyen hosszú helyre, mit
                insertTo(found,var_name_len,buff);
                izgi = 0;
                
                printf("%s\n", expr_string);
                
            }
            else{
                
                izgi = 0;
                for (int j = 0; j<10; j++){
                    var_name[j] = 0;
                }
                
            }
            
        }
        
        
        
    }
    
}


void subst_all_functions_starting_from_the_back(char* expr_string, int len){
    
    int izgi = 0;
    
    printf("Subst Fncs\n");
    
    // i must be signed int!!!!
    for(int i= len; i>=0; i--){
        
        if (izgi == 0){
            
            if (expr_string[i] == '(' ){

                izgi = 1;
                //printf("izgi=%d, i=%d\n", izgi, i);  
                
            }
            
        }
        else if (izgi == 1 || izgi == 2 || izgi == 3){
            
            if ((expr_string[i] >= '0' && expr_string[i] <= '9') || 
            (expr_string[i] >= 'a' && expr_string[i] <= 'z') || 
            (expr_string[i] >= 'A' && expr_string[i] <= 'Z') || 
            (expr_string[i] == '_')){
                

                if (izgi==1){
                    izgi = 2;
                }

                if ((expr_string[i] >= '0' && expr_string[i] <= '9')){
                    
                }
                else{
                    // nem csak szám van benne szóval fasz
                    izgi = 3;
                    
                }
                
                
                if (i==0 && izgi==3){ // start of expr string special case
                    calcSubFnc(&expr_string[i]);
                    
                }
                //printf("izgi=%d, i=%d\n", izgi, i);  
                
            }
            else if (izgi==3){
                
                calcSubFnc(&expr_string[i+1]);
  
                izgi = 0;
                //printf("izgi=%d, i=%d\n", izgi, i);  
                
            }
            else{
                //mégsem
                izgi = 0;
                //printf("izgi=%d, i=%d\n", izgi, i);  
                //printf("mégsem");
                
            }
            
        }
    }
    
}


