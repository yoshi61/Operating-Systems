#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

//this structure  is for storing mem
typedef struct node{
    int idx;    //name of the mem
    int ref_bit;    //refference bit
    int dirty;     //dirty bit
    int ref_byte; //used for ARB
    int time_in;    //time that the page has been brought in
    struct node* left; //pointing to the previous
    struct node* right; //pointing to the next
} node;

//Global variables
static int event_count = 0;
static int read_count = 0;
static int write_count = 0;
static node* current;
static node* mhead;
static node* mtail;
static char* algorithm;
static int page_size;
static int num_pages;
static int interval;
static int debug = 0;

//function declarations
#define oops(m,x) { perror(m); exit(x); } //handle errors
char *byte_to_binary(int x);//convert byte to binary
int hexadecimalToDecimal(char hexVal[]) ;// Function to convert hexadecimal to decimal
char** strSplit(char* a_str, const char a_delim);//split string with delim and return a atring array
node* createHead();//create a linked list head
node* createTail(node* head);//create a linked list tail
void initMemory(node* tail, int length);//initial a linked list as memory buffer with given size
node* create(int idx, int dirty, int ref);//create a node
int getMemorySize(node* head);//return the size of a linked list
void pushBack(node* tail, node* mem);//push given node to the back of the given linked list
void pushFront(node* head, node* mem);//insert given node at the front of the given linked list
node* removeMem(node* mem);//cut the given node out from the linked list
void printMemory(node* head);//print each entry of the given linked list
void printMem(node* mem);//print one entry
void printRes();//print the final result
void circleInc();//increment the "current" node by 1, if exceed the linked list, will wrap around automatically
int getLeftMostBit(int x);//get the left most bit of an int as binary
//Second Chance
void secondChance(int pNum, char R_W);
int insertOrUpdate(int pNum, char R_W);
int checkPageExist(int pNum, char R_W);
//Enhanced Second Chance
void enhancedSecondChance(int pNum, char R_W);
int insert2empty(int pNum, char R_W);
int find00(int pNum, char R_W);
int find01(int pNum, char R_W);
//Additional Reference Bit Algorithm
void additionalRefBits(int pNum, char R_W);
int checkPageExist2(int pNum, char R_W);
int insert2minRef(int pNum, char R_W);
int shiftRefByte();
//Enhaned Additional Reference Bit Algorithm
void enhancedAdditionalRefBits(int pNum, char R_W);
int insert2minRef2(int pNum, char R_W);

///***************************MAIN FUNCTION***************************///
int main(int argc,char **argv){
    //check argument number
    if(argc < 5){
        oops("please check your arguments!", 1);
    }

    //make two way linked list with haed and tail initialized
    //the Memory will look like this:
    //NULL<=>head<=>mem1<=>mem2<=>.....<=>memN<=>tail<=>NULL
    mhead = createHead();
    mtail = createTail(mhead);
    page_size = atoi(argv[2]);
    num_pages = atoi(argv[3]);
    algorithm = argv[4];
    //ARB and EARB will take the 6th argument
    if(strcmp(algorithm, "ARB") == 0 || strcmp(algorithm, "EARB") == 0){
        if(argc == 6){
            interval = atoi(argv[5]);
        }
        else{
            oops("please check your arguments!", 1);
        }
    }
    //declare the function pointer
    void (*fun_ptr)(int, char);

    //switch function for different algorithm using function pointer
    if(strcmp(algorithm, "SC") == 0){
        fun_ptr = &secondChance;
    }
    else if(strcmp(algorithm, "ESC") == 0){
        fun_ptr = &enhancedSecondChance;
    }
    else if(strcmp(algorithm, "ARB") == 0){
        fun_ptr = &additionalRefBits;
    }
    else if(strcmp(algorithm, "EARB") == 0){
        fun_ptr = &enhancedAdditionalRefBits;
    }
    else{
        oops("what is this algorithm??????", 3);
    }

    //initialize vertual memory size with given value
    initMemory(mtail, num_pages);
    current = mhead->right;

    if(debug){
        printf("%d, %d, %s\n",page_size, num_pages, algorithm );
    }

    FILE * fp;
    char * fname = argv[1];
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    //fopen file
    fp = fopen(fname, "r");
    if (fp == NULL)
        oops("can not read file!", 2);

    //read lines and simulate the virtual memory with the particular page replacement algorithm
    while ((read = getline(&line, &len, fp)) != -1) {
        if(line[0] == 'R' || line[0] == 'W'){
            if(debug>2){
                printf("%s", line);
            }
            event_count++;
            char** tokens = strSplit(line, ' ');
            char R_or_W = tokens[0][0];
            char* address_hex = tokens[1];
            int address_dec = hexadecimalToDecimal(address_hex);
            int page_number = address_dec / page_size;
            (*fun_ptr)(page_number, R_or_W);
            if(debug>2){
                printMemory(mhead);
            }
        }
    }
    //close file and free the memory
    fclose(fp);
    if (line) free(line);

    //print out the result statistics
    printRes();

    return 0;
}

//convert byte to binary
char *byte_to_binary(int x){
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1){
        strcat(b, ((x & z) == z) ? "1" : "0");
    }
    return b;
}

// Function to convert hexadecimal to decimal
int hexadecimalToDecimal(char hexVal[])
{
    int len = strlen(hexVal);
    // Initializing base value to 1, i.e 16^0
    int base = 1;
    int dec_val = 0;
    // Extracting characters as digits from last character
    for (int i=len-1; i>=0; i--){
        if (hexVal[i]>='0' && hexVal[i]<='9'){
            dec_val += (hexVal[i] - 48)*base;
            // incrementing base by power
            base = base * 16;
        }
        else if (hexVal[i]>='A' && hexVal[i]<='F'){
            dec_val += (hexVal[i] - 55)*base;
            // incrementing base by power
            base = base*16;
        }
        else if (hexVal[i]>='a' && hexVal[i]<='f'){
            dec_val += (hexVal[i] - 87)*base;
            // incrementing base by power
            base = base*16;
        }
    }
    return dec_val;
}

//split string with delimiter
//this function is copied from https://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c
char** strSplit(char* a_str, const char a_delim){
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    while (*tmp){
        if (a_delim == *tmp){
            count++;
            last_comma = tmp;
        }
        tmp++;
    }
    count += last_comma < (a_str + strlen(a_str) - 1);
    count++;
    result = malloc(sizeof(char*) * count);
    if (result){
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token){
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }
    return result;
}

//create a head of the Memory
node* createHead(){
    struct node* head = (node*)malloc(sizeof(node));
    head->left = NULL;
    head->idx = -1;
    return head;
}

//create a tail of the Memory
node* createTail(node* head){
    struct node* tail = (node*)malloc(sizeof(node));
    head->right = tail;
    tail->left = head;
    return tail;
}

void initMemory(node* tail, int length){
    int count = length;
    while(count >= 1){
        node* mem = create(0, -1, -1);
        pushBack(tail, mem);
        count--;
    }
}

//create a mem and return it
node* create(int idx, int dirty, int ref){
    node* new_node = (node*)malloc(sizeof(node));
    if(new_node == NULL){
        oops("Error creating a new node.\n", 3);
    }
    new_node->idx = idx;
    new_node->dirty = dirty;
    new_node->ref_bit = ref;
    new_node->ref_byte = 0; //set to 0000 0000
    new_node->time_in = INT_MAX;

    return new_node;
}

//return the size of a Memory
int getMemorySize(node* head){
    int count = 0;
    node* cursor = head;
    while(cursor->right->right != NULL){
        cursor = cursor->right;
        count++;
    }
    return count;
}

//add a mem to a Memory at the front
void pushFront(node* head, node* mem){
    head->right->left = mem;
    mem->right = head->right;
    head->right = mem;
    mem->left = head;
}

//add a mem(node) to a Memory at the back
void pushBack(node* tail, node* mem){
    tail->left->right = mem;
    mem->left = tail->left;
    tail->left = mem;
    mem->right = tail;
}

//remove mem from Memory and return the mem
node* removeMem(node* mem){
    node* res = mem;
    node* lnode = mem->left;
    node* rnode = mem->right;
    lnode->right = rnode;
    rnode->left = lnode;
    res->left = NULL;
    res->right = NULL;
    return res;
}

//print the entire Memory with its information
void printMemory(node* head){
    printf("Pointer at pNum %d:\n", current->idx);
    printf("pNum\tRef\tDirty\tRef_Byte\n");
    node* cursor = head;
    if(cursor->right == NULL || cursor->right->right == NULL){
        printf("Nothing in this Memory!\n");
        return;
    }
    cursor = cursor->right;
    while(cursor->right != NULL){
        printf("%d\t%d\t%d\t%s\n", cursor->idx, cursor->ref_bit, cursor->dirty, byte_to_binary(cursor->ref_byte));
        cursor = cursor->right;
    }
}

//print out all info of a mem
void printMem(node* mem){
    printf("This is mem s%d:\n", mem->idx);
    printf("Index\tDirty\tRef\n");
    node* cursor = mem;
    printf("%d\t%d\t%d\n", cursor->idx, cursor->dirty, cursor->ref_bit);
}

void printRes(){
    printf("events in trace:\t%d\ntotal disk reads:\t%d\ntotal disk writes:\t%d\n", event_count, read_count, write_count);
}

void circleInc(){
    current = current->right;
    //next round
    if(current->right == NULL){
        current = mhead->right;
    }
}

int getLeftMostBit(int x){
    int ct=0;
    while (x > 1) { ct++; x = x >> 1; }
    x = x << ct;
    return x;
}

//******************************srcond chance algorithm******************************************
void secondChance(int pNum, char R_W){
    if(checkPageExist(pNum, R_W) == 0){
        read_count++;
        int check = insertOrUpdate(pNum, R_W);
        if(check == 0){
            oops("somthing wrong with the page insertion or updating with SC", 6);
        }
    }
}


//check if page num exists, if so, renew the ref bit if empty entry exist, insert there
int insertOrUpdate(int pNum, char R_W){
    while(1){
        int pId = current->idx;
        if(pId == 0){               //this is an empty slot
            current->idx = pNum;
            current->ref_bit = 1;
            if(R_W == 'W'){
                current->dirty = 1;
            }
            else{
                current->dirty = 0;
            }

            if(debug){
                printf("MISS:\t page %d\n", pNum);
            }
            circleInc();
            return 1;
        }
        else{                         //check refference bit and do the page replace
            if(current->ref_bit == 0){
                if(debug){
                    printf("MISS:\t page %d\n", pNum);
                }

                if(current->dirty == 1){
                    write_count++;
                    if(debug){
                        printf("REPLACE: page %d(DIRTY)\n", current->idx);
                    }
                }
                else{
                    if(debug){
                        printf("REPLACE: page %d\n", current->idx);
                    }
                }
                current->idx = pNum;
                current->ref_bit = 1;
                if(R_W == 'W'){
                    current->dirty = 1;
                }
                else{
                    current->dirty = 0;
                }
                circleInc();
                return 1;
            }
            else{
                current->ref_bit--;
            }
        }
        //move pointer
        circleInc();
    }
    return 0;
}

//check if page number is already in the memory, if so update the ref_bit and return 1 otherwise 0
int checkPageExist(int pNum, char R_W){
    node* cursor = mhead;
    cursor = cursor->right;
    while(cursor->right != NULL){
        if(cursor->idx == pNum){
            cursor->ref_bit = 1;
            if(R_W == 'W'){
                cursor->dirty = 1;
            }
            if(debug){
                printf("HIT:\t page %d\n", pNum);
            }
            return 1;
        }
        cursor = cursor->right;
    }
    return 0;
}

//******************************enhanced srcond chance algorithm******************************************

void enhancedSecondChance(int pNum, char R_W){
    if(checkPageExist(pNum, R_W) == 0){ //check if page is already in the memory
        read_count++;
        if(debug){
            printf("MISS:\t page %d\n", pNum);
        }
        if(insert2empty(pNum, R_W)){    //find empty slot to insert
            return;
        }
        else if(find00(pNum, R_W)){     //find (0,0)
            return;
        }
        else if(find01(pNum, R_W)){     //find (0,1)
            return;
        }
        else{
            //second round to garuantee the victim to be found
            if(find00(pNum, R_W)){
                return;
            }
            else if(find01(pNum, R_W)){
                return;
            }
            else{
                oops("somthing wrong with the page insertion or updating with ESC", 7);
            }
        }
    }
}

//check if page num exists, if so, renew the ref bit if empty entry exist, insert there
int insert2empty(int pNum, char R_W){
    while(1){
        int pId = current->idx;
        if(pId == 0){
            current->idx = pNum;
            current->ref_bit = 1;
            if(R_W == 'W'){
                current->dirty = 1;
            }
            else{
                current->dirty = 0;
            }

            circleInc();
            return 1;
        }
        else{
            return 0;
        }
        //move pointer
        circleInc();
    }
    return 0;
}

//find (0,0) to replace
int find00(int pNum, char R_W){
    if(debug>1){
        printf("Finding 00 in the buffer!\n");
    }
    int count = 0;
    int originId = -1;
    //do one cycle
    while(current->idx != originId){
        if(debug>1){
            printf("Finding 00 in the buffer! at id:%d (%d, %d)\n",current->idx,current->ref_bit,current->dirty);
        }
        if(count == 0){
            originId = current->idx;
        }
        if(current->ref_bit == 0 && current->dirty == 0){
            if(debug){
                printf("REPLACE: page %d\n", current->idx);
            }
                current->idx = pNum;
            if(R_W == 'W'){
                current->idx = pNum;
                current->ref_bit = 1;
                current->dirty = 1;
            }
            else{
                current->idx = pNum;
                current->ref_bit = 1;
                current->dirty = 0;
            }
            circleInc();
            return 1;
        }
        circleInc();
        count++;
    }
    return 0;
}

//Cycle through the buffer looking for (0,0). If one is found, replace that page
int find01(int pNum, char R_W){
    if(debug>1){
        printf("***Finding 01 in the buffer!\n");
    }
    int count = 0;
    int originId = -1;
    //do one cycle
    while(current->idx != originId){
        if(debug>1){
            printf("*Finding 01 in the buffer! at id:%d (%d, %d)\n",current->idx,current->ref_bit,current->dirty);
        }
        if(count == 0){
            originId = current->idx;
        }
        if(current->ref_bit == 0 && current->dirty == 1){
            if(debug){
                printf("REPLACE: page %d (dirty)\n", current->idx);
            }
            //its guaranteed to be a write operation (dirty)
            write_count++;
            if(R_W == 'W'){
                current->idx = pNum;
                current->ref_bit = 1;
                current->dirty = 1;
            }
            else{
                current->idx = pNum;
                current->ref_bit = 1;
                current->dirty = 0;
            }
            circleInc();
            return 1;
        }
        else{
            //if the current is not 01 but 10, set it to 00
            current->ref_bit = 0;
        }
        circleInc();
        count++;
    }
    return 0;
}

//*************************************Additional Reference Bit Algorithm*************************************

//Additional Reference Bits
void additionalRefBits(int pNum, char R_W){
    //do the shift
    if(debug > 1){
        printf("event count:%d interval:%d\n", event_count, interval);
    }
    if(event_count % interval == 1 || interval == 1){
        shiftRefByte();
    }
    //check page exist || insert || replace the min ref byte page
    if(checkPageExist2(pNum, R_W)){
        return;
    }
    else if(insert2minRef(pNum, R_W)){
        read_count++;
        return;
    }
    else{
        oops("Oops, somthing went wrong in AdditionalRefBits()!", 8);
    }
}

//check if page exist in the memory, if so, renew the ref byte
int checkPageExist2(int pNum, char R_W){
    node* cursor = mhead;
    cursor = cursor->right;
    while(cursor->right != NULL){
        if(cursor->idx == pNum){
            //put '1' at the left most bit in ref_byte
            if(cursor->ref_byte < 128){
                cursor->ref_byte += 128;
            }
            if(R_W == 'W'){
                cursor->dirty = 1;
            }
            if(debug){
                printf("HIT:\t page %d\n", pNum);
            }
            return 1;
        }
        cursor = cursor->right;
    }
    return 0;
}

//find the minimum ref_byte and insert the new page in there
int insert2minRef(int pNum, char R_W){
    if(debug){
        printf("MISS:\t page %d\n", pNum);
    }
    int count = 0;
    int min = 256;
    node* minPage;
    while(count < num_pages){
        if(current->idx == 0){
            current->idx = pNum;
            current->ref_byte = 128;
            current->time_in = event_count;
            if(R_W == 'W'){
                current->dirty = 1;
            }
            else{
                current->dirty = 0;
            }
            circleInc();
            return 1;
        }
        else{
            if(current->ref_byte < min){
                minPage = current;
                min = current->ref_byte;
            }
            else if(current->ref_byte == min && current->time_in < minPage->time_in){
                minPage = current;
                min = current->ref_byte;
            }
        }
        //move pointer
        circleInc();
        count++;
    }
    if(min < 256){
        current = minPage;
        if(current->dirty == 1){
            write_count++;
        }
        if(debug){
            printf("REPLACE: page %d\n", current->idx);
        }
        current->idx = pNum;
        current->ref_byte = 128;
        current->time_in = event_count;
        if(R_W == 'W'){
            current->dirty = 1;
        }
        else{
            current->dirty = 0;
        }
        circleInc();
        return 1;
    }
    printf("Something went wrong in insert2minRef\n");
    return 0;
}

//shift ref byte to the right for all pages
int shiftRefByte(){
    node* cursor = mhead;
    cursor = cursor->right;
    while(cursor->right != NULL){
        cursor->ref_byte = (cursor->ref_byte >> 1);
        cursor = cursor->right;
    }
    return 0;
}

//*************************************Enhanced Additional Reference Bit Algorithm*************************************

//Enhanced Additional Reference Bits
void enhancedAdditionalRefBits(int pNum, char R_W){
    //do the shift
    if(debug > 1){
        printf("event count:%d interval:%d\n", event_count, interval);
    }
    if(event_count % interval == 1 || interval == 1){
        shiftRefByte();
    }
    //check page exist || insert || replace the min ref byte page
    if(checkPageExist2(pNum, R_W)){
        return;
    }
    else if(insert2minRef2(pNum, R_W)){
        read_count++;
        return;
    }
    else{
        oops("Oops, somthing went wrong in AdditionalRefBits()!", 8);
    }
}

//find the minimum ref_byte and insert the new page in there
//the ref_byte of a dirty page will beconsidered to have 3 bit left shifted value
int insert2minRef2(int pNum, char R_W){
    if(debug){
        printf("MISS:\t page %d\n", pNum);
    }
    int count = 0;
    int min = 256;
    //set current page as victim at first
    node* minPage = current;
    //do one cycle to find the victim
    while(count < num_pages){
        if(current->idx == 0){  //if empty slot, fill in and return 1
            current->idx = pNum;
            current->ref_byte = 128;
            current->time_in = event_count;
            if(R_W == 'W'){
                current->dirty = 1;
            }
            else{
                current->dirty = 0;
            }
            circleInc();
            return 1;
        }
        else{   //find victim
            int tmp_ref_byte = current->ref_byte;
            if(current->dirty){
                //the ref_byte of a dirty page will beconsidered to have 3 bit left shifted value
                if(current->ref_byte == 0){
                    tmp_ref_byte = 4;
                }
                else{
                    tmp_ref_byte = (current->ref_byte << 3);
                }
                //get the lft most bit of the shifted value
                tmp_ref_byte = getLeftMostBit(tmp_ref_byte);
                //compare with the min ref_byte and replace the victim if meets the criteria
                if(tmp_ref_byte < getLeftMostBit(min)){
                    minPage = current;
                    min = tmp_ref_byte;
                }
                else if(tmp_ref_byte == getLeftMostBit(min)){
                    if(minPage->dirty == 1 && current->time_in < minPage->time_in){
                        minPage = current;
                        min = tmp_ref_byte;
                    }
                }
            }
            else{
                if(minPage->dirty == 1){
                    //compare with the min ref_byte and replace the victim if meets the criteria
                    if(getLeftMostBit(current->ref_byte) <= min){
                        minPage = current;
                        min = current->ref_byte;
                    }
                    else if(current->ref_byte == min && minPage->dirty == 1){
                        minPage = current;
                        min = current->ref_byte;
                    }
                }
                else{
                    //compare with the min ref_byte and replace the victim if meets the criteria
                    if(current->ref_byte < min){
                        minPage = current;
                        min = current->ref_byte;
                    }
                    else if(current->ref_byte == min && current->time_in < minPage->time_in){
                        minPage = current;
                        min = current->ref_byte;
                    }
                }
            }
        }
        //move pointer
        circleInc();
        count++;
    }
    if(min < 256){
        current = minPage;
        if(current->dirty == 1){
            write_count++;
            if(debug){
                printf("REPLACE: page %d (dirty)\n", current->idx);
            }
        }
        else{
            if(debug){
                printf("REPLACE: page %d\n", current->idx);
            }
        }
        current->idx = pNum;
        current->ref_byte = 128;
        current->time_in = event_count;
        if(R_W == 'W'){
            current->dirty = 1;
        }
        else{
            current->dirty = 0;
        }
        circleInc();
        return 1;
    }
    printf("Something went wrong in insert2minRef\n");
    return 0;
}
