#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFFER_SIZE 255

//KV-pair node structure
typedef struct KVnode{
	int key;
	char *value;
    struct KVnode *next;
} KVnode;

KVnode *g_head = NULL; // starting node of list
KVnode *g_tail = NULL; // ending node of list

int g_argc;
char **g_argv;

//Global Declarations
FILE* fp;
void LoadDatabase();
KVnode *AppendKVNode(KVnode*, int, char*);
void PrintKVNodes(KVnode*);
void PutEntry(int);
void GetEntry(int);
KVnode *DeleteEntry(KVnode*, int);
void ClearEntries();
void WriteDatabase();

void PutEntry(int argno)
{
    char *arg = g_argv[argno];
    char *keytoken = strsep(&arg, ",");  //get the first token (key)
    if(keytoken == NULL)
    {
        printf("bad command\n");
        return;
    }
    char *valtoken = strsep(&arg, ",");  //get the second token (value)
    if(valtoken == NULL)
    {
        printf("bad command\n");
        return;
    }

    //handling duplication, deleting older entry
    if(g_head != NULL)
    {
        KVnode *current = g_head;
        while(current != NULL)
        {
            if(current->key == atoi(keytoken))
            {
                DeleteEntry(g_head, argno);
                break;
            }
            current = current->next;
        }
    }
    if(atoi(keytoken) == 0)
    {
        printf("bad command\n");
        return;
    }
    g_tail = AppendKVNode(g_tail, atoi(keytoken), valtoken); //append to linkedlist
}

void GetEntry(int argno)
{
    char *arg = g_argv[argno];
    char *keytoken = strsep(&arg, ",");  //get the first token (key)
    if(keytoken == NULL)
    {
        printf("bad command\n");
        return;
    }
    if(strsep(&arg, ",") != NULL)
    {
        printf("bad command\n");
        return;
    }   
    int key = atoi(keytoken);
    KVnode *current = g_head;
    while(current != NULL)
    {
        if(current->key == key)
        {
            printf("%d,%s\n",key,current->value);
            return;
        }
        current = current->next;
    }
    printf("%d not found\n",key);
}

KVnode *DeleteEntry(KVnode *head, int argno)
{
    if(head == NULL) //early exit condition: list is empty
        return NULL;
    char *arg = g_argv[argno];
    char *keytoken = strsep(&arg, ",");  //get the first token (key)
    if(keytoken == NULL)
    {
        printf("bad command\n");
        return head;
    }
    if(strsep(&arg, ",") != NULL)
    {
        printf("bad command\n");
        return head;
    }   
    int key = atoi(keytoken);

    if(head->key == key)  //early exit condition: head entry matches with item to be deleted
    {
        KVnode *temp = head;
        head = head->next;
        free(temp);
        if(head == NULL)  //extra case if list became empty after deletion
            g_tail = NULL;
        return head;
    }

    KVnode *current = head;  //search for key and delete
    while(current->next != NULL)
    {
        if(current->next->key == key)
        {
            KVnode *temp = current->next;
            current->next = temp->next;
            free(temp);
            return head;
        }
        current = current->next;
    }
    printf("%d not found\n",key);
    return head;
}

void ClearEntries()
{
   KVnode *current = g_head;
   while (current != NULL)
   {
       KVnode *next = current->next;
       free(current);
       current = next;
   }
   g_head = NULL;
   g_tail = NULL;
}

int main(int argc, char **argv)
{
    //early exit condition. No arguments specified
    if(argc == 1)
        return 0;

    //checking for existing database 'database.txt' in the current directory
    if((fp = fopen("database.txt", "r")) != NULL)  //database.txt exists
    {
        LoadDatabase(); //load database.txt
        fclose(fp);
    }

    g_argc = argc;
    g_argv = argv;
    //Execute the following for every argument
    for(int i = 1; i < argc; i++)
    {
        //Extract command key
        char *op = strsep(&argv[i], ",");
        char operation = op[0];

        switch(operation)
        {
            case 'p': PutEntry(i);
                break;
            case 'g': GetEntry(i);
                break;
            case 'd': g_head = DeleteEntry(g_head, i);
                break;
            case 'c': ClearEntries();
                break;
            case 'a': PrintKVNodes(g_head);
                break;
            default: printf("bad command\n");
        }
    }
    fp = fopen("database.txt", "w");
    WriteDatabase();
    fclose(fp);
}

void LoadDatabase()  //load database into a linkedlist
{
    char buffer[BUFFER_SIZE];
    char *keytoken, *valtoken;

    while(fgets(buffer, BUFFER_SIZE, fp) != NULL)
    {
        char *line = buffer;
        keytoken = strsep(&line, ",");  //get the first token (key)
        valtoken = strsep(&line, ",");  //get the second token (value)
        valtoken[strcspn(valtoken, "\n")] = 0;  //Removing trailing new line character in valtoken
        g_tail = AppendKVNode(g_tail, atoi(keytoken), valtoken);  //Append to Linkedlist
    }
}

void WriteDatabase() //Write linkedlist into the database
{
    if(g_head == NULL)
        return;
    KVnode *current = g_head;
    while(current != NULL)
    {
        fprintf(fp,"%d,%s\n",current->key,current->value);
        current = current->next;
    }
}

KVnode* AppendKVNode(KVnode *tail, int key, char *value)  //add to end of list
{
    // create a new node with passed key and value
    KVnode *newNode = (KVnode*) malloc(sizeof(KVnode)); //memory reserved for new node on heap
    newNode->key = key;  //set key
    newNode->value = malloc(strlen(value)+1);
    strcpy(newNode->value, value); //set value
    newNode->next = NULL; //set address of next element to new node
    if(g_tail == NULL) //first element
    {
        g_head = newNode;
        g_tail = newNode;
    }
    else //not a first element
    {
        g_tail->next = newNode; //append it to the end of the list
        g_tail = g_tail->next; //update tail pointer to end of the list
    }
    return g_tail;
}

void PrintKVNodes(KVnode *head)
{
    KVnode *current = head;
    while(current != NULL)
    {
        printf("%d,%s\n",current->key, current->value);
        current = current->next;
    }
}
