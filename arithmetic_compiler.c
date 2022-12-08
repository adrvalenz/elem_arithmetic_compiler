#include <stdio.h>
#include <stdlib.h>

typedef struct node
{   
    char lexeme;                                                    //lexeme, can be an operator or a letter representing an operand in this case
    int type;                                                       //type field, meant to discern between operand = OP, alphabet = ALPHA
    struct node *left_child;
    struct node *right_child;
    struct node *parent;
} node;
// typedef struct node node;

//globally valuable data
const int ALPHABET_LENGTH = 27;
const int OPERATOR_LENGTH = 6;
const int VALID_LEXEME_STR_LENGTH = 32;                             //length of string containing all valid lexemes
const char *valid_lexemes = "abcdefghijklmnopqrstuvwxyz+-*/=";       //string containing all valid lexemes, can be done without a finite automaton due to only single characters comprising the possible tokens
const char *valid_alphabet = "abcdefghijklmnopqrstuvwxyz";
const char *valid_operators = "+-*/=";
const int INPUT_BUFFER_SIZE = 250;
#define ALPHA -1
#define OP -2
//funcs
//Gets input from the user.
void get_input(char *);
void compiler_front_end(char *, char *);                            //Compiles the input into the intermediate represenation: three address code
void mini_lexer(char *, char *);                                    //Takes in the input string, and outputs string of only valid tokens in the input if input is valid. (sufficient, as all tokens are single character length)
void mini_parser(char *, node **);                                  //Takes in a stream of tokens, validates it within the CFG, and creates a syntax tree for the stream
void mini_addy_gen(node *, char * );                                //Takes in syntax tree, and outputs a string that is the intermediate representation
/*
    HELPER FUNCTIONS FOR ADDRESS GENERATOR
*/
char *op_node(node *, int *, char *, char *);                       //creates three address code given an operator and its children
void str_cat(char *, char *);                                       //concatenates the pascal style string intermediate representation and the added three address code line
void three_addy_line_eq(char *, char *, char *);                    //creates a string using other strings for the last assignment statement using the equal operator
void three_addy_line_w_op(char *, char *, char *, char, char *);    //creates a string using other strings and characters to create one line for the 
/*
    HELPER FUNCTIONS FOR PARSER
    Context Free grammar used:
    S -> 𝓵=A
    A -> 𝓵B
    B -> αA | ɛ
    𝓵 -> any single letter from the alphabet
    α -> + | - | * | /
*/
int grammar_police_accepted(char *);                                            //makes sure input is in language defined by cfg
int S(char *, int *);
int A(char *, int *);
int B(char *, int *);
int LETTER(char *, int *);
int OPERATOR(char *, int *);
//Syntax tree constructing functions
node *create_right_child(char *, node **, node *, int *, int, int *, int *);    //Gets the right hand child for the = operator, and its children
node *create_node(char *, node **, node *, int *, int, int *, int *);           //Create node and all ancestor nodes
node *create_leaf(char, node **, node *, int *);                                //Create leaf node for character in alphabet
int is_not_accounted_for(int *, int, int);                                      //checks if the index has already been visited in token stream

//misc functions
void set_null_string(char *, int);

//Takes a string created of a language containing only elementary arithmetic (excluding parentheses) and creates its respective three address code intermediate representation.
int main()
{
    char program[INPUT_BUFFER_SIZE + 1];
    char three_address_code[(INPUT_BUFFER_SIZE * 7)/2];
    set_null_string(program, INPUT_BUFFER_SIZE + 1);
    set_null_string(three_address_code, (INPUT_BUFFER_SIZE * 7)/2);
    three_address_code[0] = 0;  //set up to use a pascal string
    get_input(program);
    compiler_front_end(program, three_address_code);
    printf("%s", three_address_code);

    return 0;
}

void get_input(char * input_string)
{
    printf("Only elementary arithmetic, exluding parentheses and unary operations. may be used with the following symbols: *, /, +, -, and =. Maximum length is %d characters.\nEnter your string here: ", INPUT_BUFFER_SIZE);
    int size = INPUT_BUFFER_SIZE + 1;
    getline(&input_string, &size, stdin);
    printf("%s", input_string);
}

void compiler_front_end(char *raw_program, char *interm_repr)
{
    char tokens_only[INPUT_BUFFER_SIZE + 1];
    set_null_string(tokens_only, INPUT_BUFFER_SIZE + 1);
    node *syntax_tree [INPUT_BUFFER_SIZE];
    //Create nodes themselves
    for(int index; index < INPUT_BUFFER_SIZE; index++)
        syntax_tree[index] = (node *)malloc(sizeof(node));
    mini_lexer(raw_program, tokens_only);
    printf("mini lexer passed with token output: %s\n", tokens_only);  //debug
    mini_parser(tokens_only, syntax_tree);
    printf("mini parser passed with result head's lexeme being: %c", syntax_tree[0]->lexeme);
    mini_addy_gen(syntax_tree[0], interm_repr);
    //Free dynamically allocated memory
    for(int index; index < INPUT_BUFFER_SIZE; index++)
        free(syntax_tree[index]);
}

void mini_lexer(char *raw_prog, char *tokenized_input)
{   
    int tokenized_input_iterator = 0;
    int i = 0;
    while(raw_prog[i] != '\0' && i < INPUT_BUFFER_SIZE)
    {
        //if the character is a space, throw it away by skipping over it.
        if(raw_prog[i] != ' ')
        {
            //check if the character being analyzed is one of the valid characters
            for (int j = 0; j < VALID_LEXEME_STR_LENGTH; j++)
            {
                if (raw_prog[i] == valid_lexemes[j])
                {
                    tokenized_input[tokenized_input_iterator] = raw_prog[i];
                    tokenized_input_iterator++;
                    break;
                }
                //This only reached if character was not valid, and not a space. Kill the process.
                printf("Lexer error. Character %c in input is not a valid token.", raw_prog[i]);
                exit(1);
            }
        }
        printf("looping...\n");
        i++;
        if(raw_prog[i] == '\0')
            printf("null terminator found\n");
        if(i >= INPUT_BUFFER_SIZE)
            printf("input buffer size exceeded");
    }
}

void mini_parser(char *token_stream, node** syntax_tree)
{
    //make sure input is part of the language generated by the CFG, using top down parsing LL(1)
    // S -> 𝓵=A
    // A -> 𝓵B
    // B -> αA | ɛ
    // 𝓵 -> any single letter from the alphabet
    // α -> + | - | * | /
    int *free_node_ptr;  //index of the next free node.
    *free_node_ptr = 0;
    if(grammar_police_accepted(token_stream))
    {
        printf("grammar police accepted the string!\n");
        //Construct syntax tree
        int visited_indices [250];
        int *visit_index;
        *visit_index = 0;
        int i = 0;  //token being examined
        while(token_stream[i] != '=')
            i++;
        //set top level node of assignment
        syntax_tree[*free_node_ptr]->parent = NULL;
        syntax_tree[*free_node_ptr]->lexeme = token_stream[i];
        //set what = assigns to as the left child.
        syntax_tree[*free_node_ptr]->left_child = create_leaf(token_stream[i - 1], syntax_tree, syntax_tree[*free_node_ptr], free_node_ptr);
        //Add visited indices to visited tracker
        visited_indices[*visit_index] = i;       //=
        *visit_index++;
        visited_indices[*visit_index] = i - 1;   //left assign values
        *visit_index++;
        //indicate first two free nodes have been consumed
        *free_node_ptr += 2;
        syntax_tree[*free_node_ptr]->right_child = create_right_child(token_stream, syntax_tree, syntax_tree[*free_node_ptr], free_node_ptr, i, visited_indices, visit_index);
    }
    else
    {
        //exits the program if the token order is not semantically correct
        printf("Error parsing the string, string is not in language defined by the CFG:\nS -> 𝓵=A\nA -> 𝓵B\nB -> αA | ɛ\n𝓵 -> any single letter from the alphabet\nα -> + | - | * | /");
        exit(1);
    }
}

void mini_addy_gen(node *syntax_tree_head, char *intermediate_representation)
{
    int *temp_counter;
    *temp_counter = 1;
    char local_addy_line[7];
    char *temp_var_one = (char *)calloc(3, sizeof(char));
    char *temp_var_two = (char *)calloc(3, sizeof(char));
    char *left_operand = op_node(syntax_tree_head->left_child, temp_counter, intermediate_representation, temp_var_one);
    char *right_operand = op_node(syntax_tree_head->right_child, temp_counter, intermediate_representation, temp_var_two);
    free((void *)temp_var_one);
    free((void *)temp_var_two);
    three_addy_line_eq(local_addy_line, left_operand, right_operand);
    str_cat(intermediate_representation, local_addy_line);
}

//no forward declaration
void set_null_string(char *null_str, int size)
{
    for(int index = 0; index < size; index++)
        null_str[index] = '\0';
}

char *op_node(node *tree_head, int *temp_counter, char *interm_repr, char *higher_temp_var)
{
    char local_addy_line[13];
    if(tree_head->type == OP)
    {
        char *temp_var_one = (char *)calloc(3, sizeof(char));
        char *temp_var_two = (char *)calloc(3, sizeof(char));
        char *left_operand = op_node(tree_head->left_child, temp_counter, interm_repr, temp_var_one);
        char *right_operand = op_node(tree_head->right_child, temp_counter, interm_repr, temp_var_two);
        higher_temp_var[0] = 't';
        higher_temp_var[1] = (char)(48 + *temp_counter);
        *temp_counter++;
        three_addy_line_w_op(local_addy_line, higher_temp_var, left_operand, tree_head->lexeme, right_operand);
        free((void *)temp_var_one);
        free((void *)temp_var_two);
        str_cat(interm_repr, local_addy_line);
    }
    else if(tree_head->type == ALPHA)
    {
        higher_temp_var[0] = tree_head->lexeme;
        higher_temp_var[1] = '\0';
    }
    return higher_temp_var;
}

//no forward declaration
void three_addy_line_eq(char *output_str, char *left_op, char *right_op)
{
    *output_str = *left_op;
    output_str++;
    *output_str = ' ';
    output_str++;
    *output_str = '=';
    output_str++;
    *output_str = ' ';
    output_str++;
    while(*right_op)
    {
        *output_str = *right_op;
        output_str++;
        right_op++;
    }
    *output_str = '\0';
}

//no forward declaration
void three_addy_line_w_op(char *output_str, char *temp_var, char *left_op, char lexeme, char *right_op)
{
    while(*temp_var)
    {
        *output_str = *temp_var;
        output_str++;
        temp_var++;
    }
    *output_str = ' ';
    output_str++;
    *output_str = '=';
    output_str++;
    *output_str = ' ';
    output_str++;
    while(*left_op)
    {
        *output_str = *left_op;
        output_str++;
        left_op++;
    }
    *output_str = ' ';
    output_str++;
    *output_str = lexeme;
    output_str++;
    *output_str = ' ';
    output_str++;
    while(*right_op)
    {
        *output_str = *right_op;
        output_str++;
        right_op++;
    }
    *output_str = '\0';
}

void str_cat(char *str_1, char *str_2)
{
    int str_1_length = (int)str_1[0];
    str_1 += str_1_length + 1;
    if (str_1_length > 0)
    {
        *str_1 = '\n';
        str_1++;
        str_1_length++;
    }
    while(*str_2 && *str_1)
    {
        *str_1 = *str_2;
        str_2++;
        str_1++;
        str_1_length++;
    }
    str_1[0] = (char)str_1_length;
}


int is_not_accounted_for(int *accounted_indices, int questionable_index, int accounted_index)
{
    for (int it = 0; it < accounted_index; it++)
    {
        if(questionable_index == accounted_indices[it])
            return 0;
    }
    return 1;
    
}

node *create_right_child(char * token_stream, node **syntax_tree, node *parent, int *free_node_ptr, int index, int *accounted_indices, int *accounted_index)
{
    //go to the next available operator to the right, where there should be one, then check for mult/div first, then add/sub to preserve order of operations
    int temp_iterator = index + 2;
    while(token_stream[index] != '\0' && is_not_accounted_for(accounted_indices, index, *accounted_index))
    {
        if (token_stream[temp_iterator] == '/' || token_stream[temp_iterator] == '*')
        {
            accounted_indices[*accounted_index] = temp_iterator;
            *accounted_index++;
            return create_node(token_stream, syntax_tree, parent, free_node_ptr, temp_iterator, accounted_indices, accounted_index);
        }
        temp_iterator += 2;
    }
    temp_iterator = index + 2;
    while(token_stream[index] != '\0' && is_not_accounted_for(accounted_indices, index, *accounted_index))
    {
        if (token_stream[temp_iterator] == '-' || token_stream[temp_iterator] == '+')
        {
            //add operator index to accounted for indices, increment size
            accounted_indices[*accounted_index] = temp_iterator;
            *accounted_index++;
            return create_node(token_stream, syntax_tree, parent, free_node_ptr, temp_iterator, accounted_indices, accounted_index);
        }
        temp_iterator += 2;
    }
    //add the character to the accounted for indices, increment size, return leaf.
    accounted_indices[*accounted_index] = index + 1;
    *accounted_index++;
    return create_leaf(token_stream[index + 1], syntax_tree, parent, free_node_ptr);

}

node *create_node(char * token_stream, node **syntax_tree, node *parent, int *free_node_ptr, int index, int *accounted_indices, int *accounted_index)
{
    //create local copy of the free ptr to always reference this one inside this function
    int local_free_node_ptr_copy = *free_node_ptr;
    *free_node_ptr++;
    //set initial attributes
    syntax_tree[local_free_node_ptr_copy]->lexeme = token_stream[index];
    syntax_tree[local_free_node_ptr_copy]->parent = parent;
    //check for operands to right and left, by order of operations.
    int temp_iterator = index + 2;
    int found = 0;
    while(!found && token_stream[index] != '\0' && is_not_accounted_for(accounted_indices, index, *accounted_index))
    {
        if (token_stream[temp_iterator] == '/' || token_stream[temp_iterator] == '*')
        {
            syntax_tree[local_free_node_ptr_copy]->type = OP;
            accounted_indices[*accounted_index] = temp_iterator;
            *accounted_index++;
            syntax_tree[local_free_node_ptr_copy]->right_child = create_node(token_stream, syntax_tree, syntax_tree[local_free_node_ptr_copy], free_node_ptr, temp_iterator, accounted_indices, accounted_index);
            found = 1;
        }
        temp_iterator += 2;
    }
    temp_iterator = index + 2;
    while(!found && token_stream[index] != '\0' && is_not_accounted_for(accounted_indices, index, *accounted_index))
    {
        if (token_stream[temp_iterator] == '-' || token_stream[temp_iterator] == '+')
        {
            syntax_tree[local_free_node_ptr_copy]->type = OP;
            //add operator index to accounted for indices, increment size
            accounted_indices[*accounted_index] = temp_iterator;
            *accounted_index++;
            syntax_tree[local_free_node_ptr_copy]->right_child = create_node(token_stream, syntax_tree, syntax_tree[local_free_node_ptr_copy], free_node_ptr, temp_iterator, accounted_indices, accounted_index);
            found = 1;
        }
        temp_iterator += 2;
    }
    //only executes if no operand was found on right hand side.
    if(!found)
    {
        syntax_tree[local_free_node_ptr_copy]->type = ALPHA;
        accounted_indices[*accounted_index] = index + 1;
        *accounted_index++;
        syntax_tree[local_free_node_ptr_copy]->right_child = create_leaf(token_stream[index + 1], syntax_tree, syntax_tree[local_free_node_ptr_copy], free_node_ptr);
    }

    //SAME THING, BUT TO THE LEFT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    temp_iterator = index - 2;
    found = 0;
    while(!found && is_not_accounted_for(accounted_indices, index, *accounted_index))
    {
        if (token_stream[temp_iterator] == '/' || token_stream[temp_iterator] == '*')
        {
            syntax_tree[local_free_node_ptr_copy]->type = OP;
            accounted_indices[*accounted_index] = temp_iterator;
            *accounted_index++;
            syntax_tree[local_free_node_ptr_copy]->left_child = create_node(token_stream, syntax_tree, syntax_tree[local_free_node_ptr_copy], free_node_ptr, temp_iterator, accounted_indices, accounted_index);
            found = 1;
        }
        temp_iterator -= 2;
    }
    temp_iterator = index - 2;
    while(!found && is_not_accounted_for(accounted_indices, index, *accounted_index))
    {
        if (token_stream[temp_iterator] == '-' || token_stream[temp_iterator] == '+')
        {
            syntax_tree[local_free_node_ptr_copy]->type = OP;
            //add operator index to accounted for indices, increment size
            accounted_indices[*accounted_index] = temp_iterator;
            *accounted_index++;
            syntax_tree[local_free_node_ptr_copy]->left_child = create_node(token_stream, syntax_tree, syntax_tree[local_free_node_ptr_copy], free_node_ptr, temp_iterator, accounted_indices, accounted_index);
            found = 1;
        }
        temp_iterator -= 2;
    }
    //only executes if no operand was found on left hand side.
    if(!found)
    {
        syntax_tree[local_free_node_ptr_copy]->type = ALPHA;
        accounted_indices[*accounted_index] = index + 1;
        *accounted_index++;
        syntax_tree[local_free_node_ptr_copy]->left_child = create_leaf(token_stream[index + 1], syntax_tree, syntax_tree[local_free_node_ptr_copy], free_node_ptr);
    }


    //return completed node, with all fields properly set
    return syntax_tree[local_free_node_ptr_copy];
}

node *create_leaf(char token, node **syntax_tree, node *parent, int *free_node_ptr)
{
    syntax_tree[*free_node_ptr]->left_child = NULL;
    syntax_tree[*free_node_ptr]->right_child = NULL;
    syntax_tree[*free_node_ptr]->lexeme = token;
    syntax_tree[*free_node_ptr]->parent = parent;
    return syntax_tree[*free_node_ptr];
}

int grammar_police_accepted(char *string)
{
    //Begin at top of CFG: S.
    printf("Parsing string... top down\n");
    int *offset;
    *offset = 0;
    return S(string, offset);
}

int S(char * string, int *offset)
{
    // S -> 𝓵=A
    if(LETTER(string, offset))
    {
        *offset++;
        printf("offset checking: %c", *offset);
        if(string[*offset] != '\0' && string[*offset] == '=')
        {
            *offset++;
            if(A(string, offset))
            {
                return 1;
            }
        }
        printf("after the if statement here\n");
    }
    printf("S completed");
    //Failure to follow CFG
    return 0;
}
int A(char * string, int *offset)
{
    // A -> 𝓵B
    if(LETTER(string, offset))
    {
        *offset++;
        if(B(string, offset))
        {
            return 1;
        }
    }
    //Failure to follow CFG
    return 0;
}
int B(char * string, int *offset)
{
    //B -> ɛ
    if(string[*offset] == '\0')
    {
        return 1;
    }
    // B -> αA
    else if(OPERATOR(string, offset))
    {
        *offset++;
        if(A(string, offset))
        {
            return 1;
        }
    }
    //Failure to follow CFG
    return 0;
}
int LETTER(char * string, int *offset)
{
    for (int i = 0; i < ALPHABET_LENGTH; i++)
    {
        if(string[*offset] == valid_alphabet[i])
        {
            return 1;
        }
    }
    //if reached here, is not part of alphabet.
    return 0;
}
int OPERATOR(char * string, int *offset)
{
    for (int i = 0; i < OPERATOR_LENGTH; i++)
    {
        if(string[*offset] == valid_operators[i])
        {
            return 1;
        }
    }
    //if reached here, is not part of valid operators.
    return 0;
}