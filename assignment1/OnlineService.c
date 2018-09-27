#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

//global variables
int tQuantum1 = 5;
int tQuantum2 = 10;
int tQuantum3 = 20;
int maxRun1 = 5;
int maxRun2 = 2;
int TIME = 0;
//this structure  is for storing customer
typedef struct node{
    char* idx;    //name of the customer
    int priority;   //the priority to be served
    int arrival;    //stores the time stamp when the customer arrives
    int age;    //when other customer get served once, this customer ages
    int end;    //stores the time stamp when the customer finishs all the excution
    int ready;  //stores the time stamp when the customer is served for the first time
    int cost;   //totale CPU time required
    int remain; //the remaining CPU time needed
    int waiting;    //the total time waited by the customer
    int timer;  //recording the time quantum has been done
    int run;    //stores the number of round that the customer has been served
    struct node* left; //pointing to the previous
    struct node* right; //pointing to the next
} node;
//function declarations
#define oops(m,x) { perror(m); exit(x); } //handle errors
char** strSplit(char* a_str, const char a_delim);
node* createHead(char* id);
node* createTail(node* head);
node* create(char* idx, int priority, int arrival, int cost);
int getQueueSize(node* head);
void addToQueue(node* head ,node* tail, node* customer);
void autoSelectQToAdd(node* q1h ,node* q1t, node* q2t, node* q3t, node* customer);
void pushBack(node* tail, node* customer);
void pushFront(node* head, node* customer);
node* removeCustomer(node* customer);
void printQueue(node* head);
void printAllQueueInfo(node* head);
void printCustomer(node* cus);
void printRes(node* head);
int getNumOfUnservedCustomer(node* q1, node* q2, node* q3, node* q4);
void incrementAge(node* q2, node* q3);
node* getNewcomer(node* q4);
void arrangeCurrentCustomer(node* q1h, node* q1t, node* q2h, node* q2t, node* q3h, node* q3t);
void arrangePromotion(node* q1h, node* q1t, node* q2h, node* q2t, node* q3h, node* q3t);
void serveNextCustomer(node* q1h, node* q1t, node* q2h, node* q2t, node* q3h, node* q3t, node* resultQueueTail);

///***************************MAIN FUNCTION***************************///
int main(int argc,char **argv){
    //make 4 of two way linked list with haed and tail initialized
    //the queue will look like this:
    //NULL<=>head<=>customer1<=>customer2<=>.....<=>customerN<=>tail<=>NULL
    node* q1head = createHead("Q1");
    node* q2head = createHead("Q2");
    node* q3head = createHead("Q3");
    node* qArvHead = createHead("Arriving Q");
    node* qResHead = createHead("Result Q");
    node* q1tail = createTail(q1head);
    node* q2tail = createTail(q2head);
    node* q3tail = createTail(q3head);
    node* qArvTail = createTail(qArvHead);
    node* qResTail = createTail(qResHead);
    int debug = 0;

    //check argument number
    if(argc < 2){
        oops("please give an input file name!", 1);
    }
    else if(argc > 2){
        debug = atoi(argv[2]);
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

    //read lines and put them all into seperated linked list according to their priority
    while ((read = getline(&line, &len, fp)) != -1) {
        char** tokens = strSplit(line, ' ');
        char* name = tokens[0];
        int arrival = atoi(tokens[1]);
        int priority = atoi(tokens[2]);
        int age = atoi(tokens[3]);
        int cost = atoi(tokens[4]);

        //create and add customers to each queue by priority
        node* newCustomer = create(name, priority, arrival, cost);
        if(arrival > 0){
            pushBack(qArvTail, newCustomer);
        }
        else if(priority < 3){
            pushBack(q3tail, newCustomer);
        }
        else if(priority >= 5){
            addToQueue(q1head, q1tail, newCustomer);
        }
        else{
            pushBack(q2tail, newCustomer);
        }
    }
    fclose(fp);
    if (line) free(line);

    //Now starting serving customers!
    //Loop until no more customer is in Q1,Q2,Q3 and the arrival Q
    while(getNumOfUnservedCustomer(q1head, q2head, q3head, qArvHead) > 0){
        //First, check if there is any new arrival
        node* newArrival = getNewcomer(qArvHead);
        //Add new arrivals to the queues
        while(newArrival != NULL){
            autoSelectQToAdd(q1head, q1tail, q2tail, q3tail, newArrival);
            newArrival = getNewcomer(qArvHead);
        }

        //Second, check if there is any customer has done their time quantum or has been interrupted
        //if so, move the customer to the end of the queue (may demote)
        arrangeCurrentCustomer(q1head, q1tail, q2head, q2tail, q3head, q3tail);

        //Thired, check if there is any customer need to be promoted
        //if so, remove the customer from its original Q and put into upper level Q
        arrangePromotion(q1head, q1tail, q2head, q2tail, q3head, q3tail);

        //print out the detail if debug is set
        if(debug){
            printf("******************************************\n");
            printf("The last moment of time(%d)\n", TIME);
            printAllQueueInfo(q1head);
            printAllQueueInfo(q2head);
            printAllQueueInfo(q3head);
            if(TIME == debug){
                return 0;
            }
        }

        //Fnally, serve the next customer. if it has complete, move it to resQ
        serveNextCustomer(q1head, q1tail, q2head, q2tail, q3head, q3tail, qResTail);


        TIME++;
    }

    //print out result
    printRes(qResHead);
    return 0;
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

    /* Count how many elements will be extracted. */
    while (*tmp){
        if (a_delim == *tmp){
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
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

//create a head of the queue
node* createHead(char* id){
    struct node* head = (node*)malloc(sizeof(node));
    head->left = NULL;
    head->idx = id;
    return head;
}

//create a tail of the queue
node* createTail(node* head){
    struct node* tail = (node*)malloc(sizeof(node));
    head->right = tail;
    tail->left = head;
    return tail;
}

//create a customer and return it
node* create(char* idx, int priority, int arrival, int cost){
    node* new_node = (node*)malloc(sizeof(node));
    if(new_node == NULL){
        oops("Error creating a new node.\n", 3);
    }
    new_node->idx = idx;
    new_node->priority = priority;
    new_node->arrival = arrival;
    new_node->age = 0;
    new_node->end = -1;
    new_node->ready = -1;
    new_node->cost = cost;
    new_node->remain = cost;
    new_node->waiting = 0;
    new_node->timer = 0;

    return new_node;
}

//return the size of a queue
int getQueueSize(node* head){
    int count = 0;
    node* cursor = head;
    while(cursor->right->right != NULL){
        cursor = cursor->right;
        count++;
    }
    return count;
}

//return the nomber of getNumOfUnservedCustomer customers
int getNumOfUnservedCustomer(node* q1, node* q2, node* q3, node* q4){
    int res = getQueueSize(q1)+getQueueSize(q2)+getQueueSize(q3)+getQueueSize(q4);
    return res;
}

//add node to queue 1 in a logical order
void addToQueue(node* head ,node* tail, node* customer){
    node* cursor = tail->left;
    if(getQueueSize(head) == 0){
        cursor->right->left = customer;
        customer->right = cursor->right;
        cursor->right = customer;
        customer->left = cursor;
        return;
    }
    while(cursor->left != NULL){
        if(cursor->priority >= customer->priority){
            cursor->right->left = customer;
            customer->right = cursor->right;
            cursor->right = customer;
            customer->left = cursor;
            return;
        }
        cursor = cursor->left;
    }
    //if there is no customer who has same or larger priority in the queue, put at front
    //also set the previous first customer's timer to 0 and increment the run
    if(head->right->timer > 0){
        head->right->timer = 0;
        head->right->run++;
    }
    pushFront(head, customer);

}

//automatically adding a customer to a queue according to its priority
void autoSelectQToAdd(node* q1h ,node* q1t, node* q2t, node* q3t, node* customer){
    if(customer->priority < 3){
        pushBack(q3t, customer);
        //printf("adding %s to Q3\n",customer->idx);
    }
    else if(customer->priority >= 5){
        //customers in Q1 does not have age
        customer->age = 0;
        addToQueue(q1h, q1t, customer);
        //printf("adding %s to Q1\n",customer->idx);
    }
    else{
        pushBack(q2t, customer);
        //printf("adding %s to Q2\n",customer->idx);
    }
}

//add a customer to a queue at the front
void pushFront(node* head, node* customer){
    head->right->left = customer;
    customer->right = head->right;
    head->right = customer;
    customer->left = head;
}

//add a customer(node) to a queue at the back
void pushBack(node* tail, node* customer){
    tail->left->right = customer;
    customer->left = tail->left;
    tail->left = customer;
    customer->right = tail;
}

//remove customer from queue and return the customer
node* removeCustomer(node* customer){
    node* res = customer;
    node* lnode = customer->left;
    node* rnode = customer->right;
    lnode->right = rnode;
    rnode->left = lnode;
    res->left = NULL;
    res->right = NULL;
    return res;
}

//print the entire queue with its information
void printQueue(node* head){
    printf("This is Queue %s:\n", head->idx);
    printf("Index\tArrival\tPrio\tage\tCPU_Time\n");
    node* cursor = head;
    if(cursor->right == NULL || cursor->right == NULL){
        printf("Nothing in this queue!\n");
        return;
    }
    cursor = cursor->right;
    while(cursor->right != NULL){
        printf("%s\t%d\t%d\t%d\t%d\n", cursor->idx, cursor->arrival, cursor->priority, cursor->age, cursor->cost);
        cursor = cursor->right;
    }
}

//print all inf of each customer in the queue
void printAllQueueInfo(node* head){
    printf("This is Queue %s:\n", head->idx);
    printf("Idx\tArvl\tPrio\tage\tend\tready\tcost\tremain\twait\ttimer\trun\n");
    node* cursor = head;
    if(cursor->right == NULL || cursor->right == NULL){
        printf("Nothing in this queue!\n");
        return;
    }
    cursor = cursor->right;
    while(cursor->right != NULL){
        printf("%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", cursor->idx, cursor->arrival, cursor->priority, cursor->age, cursor->end, cursor->ready, cursor->cost, cursor->remain, cursor->waiting, cursor->timer, cursor->run);
        cursor = cursor->right;
    }
}

//print out all info of a customer
void printCustomer(node* cus){
    printf("This is customer s%s:\n", cus->idx);
    printf("Idx\tArvl\tPrio\tage\tend\tready\tcost\tremain\twait\n");
    node* cursor = cus;
    printf("%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", cursor->idx, cursor->arrival, cursor->priority, cursor->age, cursor->end, cursor->ready, cursor->cost, cursor->remain, cursor->waiting);
}

//print the result queue with each customer's information
void printRes(node* head){
    printf("Index\tPriority\tArrival\tEnd\tReady\tCPU_Time\tWaiting\n");
    node* cursor = head;
    if(cursor->right == NULL || cursor->right == NULL){
        printf("Nothing in this queue!\n");
        return;
    }
    cursor = cursor->right;
    while(cursor->right != NULL){
        printf("%s\t%d\t%d\t%d\t%d\t%d\t%d\n", cursor->idx, cursor->priority, cursor->arrival, cursor->end, cursor->ready, cursor->cost, cursor->waiting);
        cursor = cursor->right;
    }
}

//increment the age of all customers in Q2 Q3
void incrementAge(node* q2, node* q3){
    node* cursor2 = q2;
    node* cursor3 = q3;
    while(cursor2->right->right != NULL){
        cursor2 = cursor2->right;
        cursor2->age++;
    }
    while(cursor3->right->right != NULL){
        cursor3 = cursor3->right;
        cursor3->age++;
    }
}

//return the new arrivle customer
node* getNewcomer(node* q4){
    node* cursor = q4;
    while(cursor->right->right != NULL){
        cursor = cursor->right;
        if(cursor->arrival == TIME){
            cursor = cursor->right;
            return removeCustomer(cursor->left);
        }
    }
    return NULL;
}

//check if there is any customer has been interrupted
//if so, move the customer to the end of the queue (may demote)
void arrangeCurrentCustomer(node* q1h, node* q1t, node* q2h, node* q2t, node* q3h, node* q3t){
    if(getQueueSize(q1h) > 0){
        //if the customer has done the max time quantum move to the end
        if(q1h->right->timer >= tQuantum1){
            node* doneCustomer = removeCustomer(q1h->right);
            doneCustomer->timer = 0;
            doneCustomer->run++;
            if(doneCustomer->run >= maxRun1){
                doneCustomer->run = 0;
                doneCustomer->priority--;
            }
            autoSelectQToAdd(q1h, q1t, q2t, q3t, doneCustomer);
            return;
        }
        else if(q1h->right->timer > 0){
            return;
        }
        else if(getQueueSize(q2h) > 0 && q2h->right->timer != 0){
            node* interruptedCustomer = removeCustomer(q2h->right);
            //set timer back to 0
            interruptedCustomer->timer = 0;
            //put to the end of the queue
            autoSelectQToAdd(q1h, q1t, q2t, q3t, interruptedCustomer);
            return;
        }
        else if(getQueueSize(q3h) > 0 && q3h->right->timer != 0){
            node* interruptedCustomer = removeCustomer(q3h->right);
            //set timer back to 0
            interruptedCustomer->timer = 0;
            //put to the end of the queue
            autoSelectQToAdd(q1h, q1t, q2t, q3t, interruptedCustomer);
            return;
        }
        else{
            return;
        }
    }
    else if(getQueueSize(q2h) > 0){
        if(q2h->right->timer >= tQuantum2){
            node* doneCustomer = removeCustomer(q2h->right);
            doneCustomer->timer = 0;
            doneCustomer->run++;
            if(doneCustomer->run >= maxRun2){
                doneCustomer->run = 0;
                doneCustomer->priority--;
            }
            autoSelectQToAdd(q1h, q1t, q2t, q3t, doneCustomer);
            return;
        }
        if(q2h->right->timer > 0){
            return;
        }
        else if(getQueueSize(q3h) > 0 && q3h->right->timer != 0){
            node* interruptedCustomer = removeCustomer(q3h->right);
            //set timer back to 0
            interruptedCustomer->timer = 0;
            //put to the end of the queue
            autoSelectQToAdd(q1h, q1t, q2t, q3t, interruptedCustomer);
            return;
        }
        else{
            return;
        }
    }
    else{
        if(q3h->right->timer >= tQuantum3){
            node* doneCustomer = removeCustomer(q3h->right);
            doneCustomer->timer = 0;
            autoSelectQToAdd(q1h, q1t, q2t, q3t, doneCustomer);
            return;
        }
    }
}

//Thired, check if there is any customer need to be promoted
//if so, remove the customer from its original Q and put into upper level Q
void arrangePromotion(node* q1h, node* q1t, node* q2h, node* q2t, node* q3h, node* q3t){
    node* cursor = NULL;
    //find candidate in Q2
    if(getQueueSize(q2h) > 0){
        cursor = q2h->right;
        while(cursor->right != NULL){
            if(cursor->age > 7){
                //increment its priority and reset the age to 0
                cursor->priority++;
                cursor->age = 0;
                //check if the customer need to change queue
                if(cursor->priority == 5){
                    //move to the next customer
                    cursor = cursor->right;
                    //remove the last customer from the quere as candidate
                    node* candidate = removeCustomer(cursor->left);
                    //put the candidate to a faster queue
                    autoSelectQToAdd(q1h, q1t, q2t, q3t, candidate);
                }
            }
            else{
                //move to the next customer
                cursor = cursor->right;
            }
        }
    }

    //find candidate in Q3
    if(getQueueSize(q3h) > 0){
        cursor = q3h->right;
        while(cursor->right != NULL){
            if(cursor->age > 7){
                //increment its priority and reset the age to 0
                cursor->priority++;
                cursor->age = 0;
                //check if the customer need to change queue
                if(cursor->priority == 3){
                    //move to the next customer
                    cursor = cursor->right;
                    //remove the last customer from the quere as candidate
                    node* candidate = removeCustomer(cursor->left);
                    //put the candidate to a faster queue
                    autoSelectQToAdd(q1h, q1t, q2t, q3t, candidate);
                }
            }
            else{
                //move to the next customer
                cursor = cursor->right;
            }
        }
    }
}

//This function will serve the next customer, update all customer's info
void serveNextCustomer(node* q1h, node* q1t, node* q2h, node* q2t, node* q3h, node* q3t, node* resultQueueTail){
    node* nextCustomer = NULL;
    if(getQueueSize(q1h) > 0){
        nextCustomer = removeCustomer(q1h->right);
        //update nextCustomer information
        if(nextCustomer->ready < 0){
            nextCustomer->ready = TIME;
        }
        nextCustomer->age = 0;
        nextCustomer->remain--;
        nextCustomer->timer++;

        //if cup time has been complete, put to result Q
        if(nextCustomer->remain == 0){
            nextCustomer->end = TIME+1;
            //update the waiting time
            nextCustomer->waiting = nextCustomer->end - nextCustomer->ready - nextCustomer->cost;
            pushBack(resultQueueTail,nextCustomer);
            incrementAge(q2h, q3h);
        }
        else if(nextCustomer->timer >= tQuantum1){
            incrementAge(q2h, q3h);
            //put the nextCustomer back
            pushFront(q1h,nextCustomer);
        }
        else{
            //put the nextCustomer back
            pushFront(q1h,nextCustomer);
        }
    }
    else if(getQueueSize(q2h) > 0){
        nextCustomer = removeCustomer(q2h->right);
        //update nextCustomer information
        if(nextCustomer->ready < 0){
            nextCustomer->ready = TIME;
        }
        nextCustomer->age = 0;
        nextCustomer->remain--;
        nextCustomer->timer++;

        //if cup time has been complete, put to result Q
        if(nextCustomer->remain == 0){
            nextCustomer->end = TIME+1;
            //update the waiting time
            nextCustomer->waiting = nextCustomer->end - nextCustomer->ready - nextCustomer->cost;
            pushBack(resultQueueTail,nextCustomer);
            incrementAge(q2h, q3h);
        }
        else if(nextCustomer->timer >= tQuantum2){
            incrementAge(q2h, q3h);
            //put the nextCustomer back
            pushFront(q2h,nextCustomer);
        }
        else{
            //put the nextCustomer back
            pushFront(q2h,nextCustomer);
        }
    }
    else if(getQueueSize(q3h) > 0){
        nextCustomer = removeCustomer(q3h->right);
        //update nextCustomer information
        if(nextCustomer->ready < 0){
            nextCustomer->ready = TIME;
        }
        nextCustomer->age = 0;
        nextCustomer->remain--;
        nextCustomer->timer++;

        //if cup time has been complete, put to result Q
        if(nextCustomer->remain == 0){
            nextCustomer->end = TIME+1;
            //update the waiting time
            nextCustomer->waiting = nextCustomer->end - nextCustomer->ready - nextCustomer->cost;
            pushBack(resultQueueTail,nextCustomer);
            incrementAge(q2h, q3h);
        }
        else if(nextCustomer->timer >= tQuantum3){
            incrementAge(q2h, q3h);
            //put the nextCustomer back
            pushFront(q3h,nextCustomer);
        }
        else{
            //put the nextCustomer back
            pushFront(q3h,nextCustomer);
        }
    }
    else{
        oops("serveNextCustomer() is called but there is no customer!", 4);
    }
}
