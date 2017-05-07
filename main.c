#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "sys/wait.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define DEBUG_MODE 0

struct BackgroudProcess {
    struct BackgroudProcess *pNext;
    struct BackgroudProcess *pPrev;
    pid_t pid;
    int count;
    int alive;//0 is live,0 is finished
};
typedef struct BackgroudProcess BProcces;

BProcces *pHead;
BProcces *pTail;

struct History {//hold history//Double link-list
    struct History *pNext;
    struct History *pPrev;
    char command[100];
    char inputBuffer[MAX_LINE];
    
};
struct Hist {
    struct History * head;
    struct History * tail;
    int limit;
    //hist -set num
};
struct Hist Hi;
typedef struct History History;
typedef struct History *PHistory;
History *hHead;
History *hTail;
struct Pipe {
    char inputBuffer[MAX_LINE];
    int startIndex;
    int lock;
    int count;
    //hist -set num
};
struct Pipe Pipe;

/*
 ls -l | tail -1 | cut -d " " -f 1
 */


int setup(char inputBuffer[], char *args[],int *background,int flag,char saveInputBuffer[])
{
    int length, /* # of characters in the command line */
    i,      /* loop index for accessing inputBuffer array */
    start,  /* index where beginning of next command parameter is */
    ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
    
    /* read what the user enters on the command line */
    if(flag==0){
        length = read(STDIN_FILENO,inputBuffer,MAX_LINE);
        
    }
    else if (flag==2 | flag==3)
        length=strlen(inputBuffer);
    
    
    if(saveInputBuffer!=NULL)
        strcpy(saveInputBuffer, inputBuffer);
    
    
    /* 0 is the system predefined file descriptor for stdin (standard input),
     which is the user's screen in this case. inputBuffer by itself is the
     same as &inputBuffer[0], i.e. the starting address of where to store
     the command that is read, and length holds the number of characters
     read in. inputBuffer is not a null terminated C-string. */
    
    start = -1;
    if (length == 0){
        fprintf(stderr,"by by \n");
        exit(0);   /* ^d was entered, end of user command stream */
    }
    else if(length==1)
        return -1;
    
    /* the signal interrupted the read system call */
    /* if the process is in the read() system call, read returns -1
     However, if this occurs, errno is set to EINTR. We can check this  value
     and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }
    
    //printf(">>%s<<",inputBuffer);
    for (i=Pipe.startIndex;i<length;i++){ /* examine every character in the inputBuffer */
        
        switch (inputBuffer[i]){
            case ' ':
            case '\t' :               /* argument separators */
                if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;
                
            case '\n':                 /* should be the final char examined */
                if (start != -1){
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
                i=999;
                break;
                
            default :             /* some other character */
                if (start == -1)
                    start = i;
                if (inputBuffer[i] == '&'){
                    *background  = 1;
                    inputBuffer[i-1] = '\0';
                    i=1000;
                    ct=ct+1;
                }
                else if(inputBuffer[i] == '|')
                {
                    //inputBuffer[i+1] = '\0';
                    if(Pipe.startIndex==0 & Pipe.lock==0)//save orginal input buffer
                        strcpy(Pipe.inputBuffer, saveInputBuffer);
                    
                    Pipe.startIndex=i+1;
                    Pipe.lock=1;
                    Pipe.count=Pipe.count+1;
                    if(flag==3)
                        i=999;
                    
                    //i=999;
                }
                
        } /* end of switch */
    }    /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */
#if DEBUG_MODE
    for (i = 0; i <= ct; i++)
        fprintf(stderr,"args %d = %s\n",i,args[i]);
#endif
    
    
    
    return ct;
    
}

int cmpStrings(const char *a, const char *b)//compare two string
{
    if(a==NULL | b==NULL)
        return 0;
    if(strncmp(a, b, strlen(b)) == 0) return 1;
    return 0;
}

char* concatString(const char *s1, const char *s2)//to join strings
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}


const char *builtCommands[] = {"cd","dir","clr","wait","hist","exit","!"};
int cmpStrings(const char *a, const char *b);
void systemCommand(char *args[],int lastindex,int background);
int systemCommandPipe(char *args[],int lastindex,int background,char *args2[],int counter);
void builtinCommand(char *args[],int index);
void executeChild(char *args[],int ct);
char *currentpath;//pointer refers current directory always
void AppendNodeH(History *hNode);
void AppendNodeProcces(pid_t p);
int checkBorS(char *args[]);
PHistory checkHist(char *c);
int pipeCommand(int count);
void DeleteAllHistory();


void addHistory(int lastindex,char *args[],char *point,int background);

int main(void)
{
    char inputBuffer[MAX_LINE];
    char orginalBuffer[MAX_LINE];/*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /*command line arguments */
    Hi.limit=10;
    Pipe.startIndex=0;
    Pipe.lock=0;
    Pipe.count=0;

    
    char cwd[1024];//array hold to current directory
    
    
    
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        currentpath=cwd;
    
    int lastindex=0;
    while (1){
        
        Pipe.startIndex=0;
        Pipe.lock=0;
        Pipe.count=0;
        chdir(currentpath);
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            //fprintf(stderr, "-----------: %s\n", cwd);
            
        background = 0;
        fprintf(stderr,"CSE333sh: ");
        
        lastindex=setup(inputBuffer, args, &background,0,orginalBuffer);
        if(lastindex==-1)//Check for null input
            continue;
        
        int flag=checkBorS(args);//check Builtin or System Command
        if(flag==-2)
             continue;
        
        
        if(flag!=6)
            addHistory(lastindex-background,args,orginalBuffer,background);
        
        if(flag==-1 & Pipe.lock==0)//it is shell command
            systemCommand(args, lastindex, background);
        else if(Pipe.lock==0)
            builtinCommand(args,flag);
        else if(Pipe.lock==1)//Multiple Command with Pipe
        {
            if(pipeCommand(Pipe.count)==-1)
                perror("Failed to setup pipeline");
        }
        
        
    }
    
}
int pipeCommand(int count)
{
    Pipe.startIndex=0;
    
    char *args[MAX_LINE/2 + 1];
    
    int backgroud;
    int lastindex;
    
    char inputBuffer[MAX_LINE];
    
    pid_t childpid;
    int numofCommands=count+1;
    
    int fd[2];

    
    int i;
    
    int oldPipe=0;
    for(i=0;i<numofCommands;i++)
    {
        
         if(pipe(fd) < 0)
              perror("Failed to setup pipeline");
        
        
        strcpy(inputBuffer, Pipe.inputBuffer);
        lastindex=setup(inputBuffer, args, &backgroud,3,NULL);
        if (((childpid = fork()) == -1)) {
            perror("Failed to setup pipeline");
            return 1;
        }
        if (childpid == 0)
        {
           
            
            dup2(oldPipe, 0);//0 for stdın,
          
            
            
            
            if(i!=numofCommands-1)//if not last command
            {
                if (dup2(fd[1], STDOUT_FILENO) == -1){
                    perror("Failed to redirect stdout of ls");
                }
            }
       
        
            
            close(fd[0]);
            
            
            executeChild(args,lastindex);
            return 1;
            
            
        }
        else
        {
         
            if(waitpid(childpid, NULL, 0)== -1 ){
                perror("Patlayi ne yapcaz :D");
                return 1;
            }
            oldPipe = fd[0];
            close(fd[1]);
            
            
        }
        
    }
    
    
    return 0;
}
int checkBorS(char *args[])
{
    
    if(args[0]==NULL)
        return -2;
    int b;
    for(b=0;b<=6;b++)
    {
        if(strcmp(args[0], builtCommands[b])==0)//it is builtin command
        {
            return b;
            
        }
    }
    return -1;
}
void addHistory(int lastindex,char *args[],char *point,int background)
{
    
    char *c=NULL;;
    int i;
    char *result;;
    
    if(lastindex==1)
        c=args[0];
    else
    {
        
        c=args[0];
        c=concatString(c, " ");
        char *temp;
        for(i=1;(i)<lastindex;i++)
        {
            
            temp=concatString(args[i]," ");
            
            c=concatString(c,temp);
            
        }
        
    }

    if(background==1)
        c=concatString(c," &");
    History *pNode;
    
    pNode = (History *)calloc(1,sizeof(History));
    
    strncpy(pNode->command, c,strlen(c));
    
    
    strcpy(pNode->inputBuffer,point);
    
    AppendNodeH(pNode);
    Hi.head=hHead;
    Hi.tail=hTail;
    
}



void systemCommand(char *args[],int lastindex,int background)
{

    pid_t childpid;
    childpid = fork();
    int valuereturn=0;
    if (childpid == -1) {
        perror("Failed to fork");
        //return 1;
    }
    if (childpid >0 ) {                          // parent code /
        
        if(!background)//background procces
        {
            
            if(waitpid(childpid, &valuereturn, 0)== -1 ){
                perror("Parent Failed to wait child");
                
            }
#if DEBUG_MODE
            fprintf(stderr, "return value %d \n", valuereturn);
#endif
            
            
        }
        else{
            
            AppendNodeProcces(childpid);
            BProcces *pNode;
            for (pNode = pHead; pNode != NULL; pNode = pNode->pNext) {
                //printf("%ld\n", pNode->pid);
                fprintf(stderr, "[%d], %ld \n", pNode->count,pNode->pid);
            }
            
        }
        
    }
    if (childpid == 0) {// child code /
        
        executeChild(args,lastindex);
        
    }
    
    
}

void builtinCommand(char *args[],int index)
{
    char *args_temp[MAX_LINE/2 + 1];
    
    
    if(index==0)//cd
    {
        
#if DEBUG_MODE
        int i;
        for (i = 0; i < strlen(args); i++)
            fprintf(stderr,"args %d = %s\n",i,args[i]);
#endif
        
        
        if(args[1]==NULL)
        {
            strcpy(currentpath, getenv("HOME"));
        }
        else if(cmpStrings(args[1],"/")==1){
            strcpy(currentpath, args[1]);
          
        }
        else{
            
            char *temp=concatString(currentpath,"/");
            
            temp=concatString(temp,args[1]);
            strcpy(currentpath,temp);
            
        }
        
        if(chdir(currentpath)!=0)
            fprintf(stderr,"Invalid Directory \n");
        
    }
    else if(index==1)
    {
        fprintf(stderr, "%s\n", currentpath);
    }
    else if(index==2)
    {
        system("clear");
    }
    else if(index==3)
    {
        
        pid_t ch_pid;
        while((ch_pid=wait(NULL)) > 0) { // wait for all of your children
            fprintf(stderr, "Done  ID:%ld\n",(long)ch_pid);
        }
        
        DeleteAllHistory();
        
        
        
    }
    else if(index==4)
    {
        char temp[MAX_LINE];

        if(cmpStrings(args[1],"-set"))
        {
            
            char *ptr;
            long ret;
            
            ret = strtol(args[2], &ptr, 10);
            
            
            if(ret==0)
                fprintf(stderr,"Invalid input \n");
            else
                Hi.limit=ret;
            
        }
        else
        {
            History *pNode;
            pNode = Hi.tail;
            int counter=1;
            while(pNode != NULL & counter<=Hi.limit)
            {
               ;
                fprintf(stderr,"%s  \n", pNode->command);
             
                pNode = pNode->pPrev;
                counter++;
            }
            
        }
        
    }
    else if(index==6)
    {
        
        
        char orginalBuffer[MAX_LINE];
        History *pNode;
        
        pNode=checkHist(args[1]);

        if(pNode==NULL){//hist içi boş ise
            fprintf(stderr, "Input is invalid  \n");
            return ;
        }
        
        Pipe.startIndex=0;
        Pipe.lock=0;
        Pipe.count=0;
        int background;
        int lastindex;
        background = 0;
        lastindex = 0;
        char temp[MAX_LINE];
        
        strcpy(temp, pNode->inputBuffer);
        lastindex=setup(temp,args_temp , &background, 2,orginalBuffer);
        
        
        int flag=checkBorS(args_temp);
        
        if(flag!=6)
            addHistory(lastindex-background,args_temp,orginalBuffer,background);
        
        if(flag==-1 & Pipe.lock==0)//it is shell command
            systemCommand(args_temp, lastindex, background);
        else if(Pipe.lock==0)
            builtinCommand(args_temp,flag);
        else if(Pipe.lock==1)
        {
            pipeCommand(Pipe.count);
        }
        
       
        
    }
    else if(index==5)
    {
        BProcces *pNode;
        for (pNode = pHead; pNode != NULL; pNode = pNode->pNext) {
            //printf("%ld\n", pNode->pid);
            int flag=0;
            
            pid_t result = waitpid(pNode->pid, NULL, WNOHANG);
            if (result == 0) {
                // Child still alive
                flag=1;
                pNode->alive=1;
            } else if (result == -1) {
                // Error
                
                //pNode->alive=-1;
            } else {
                // Child exited
                flag=0;
                pNode->alive=0;//bitti
            }
            
            fprintf(stderr, " %d, %ld  %d \n", pNode->count,pNode->pid,pNode->alive);
            if(flag==1)
            {
                fprintf(stderr, "You must wait all child,\n");
                return;
                
                
            }
            else
                exit(0);
            
        }

        exit(0);
    }
}


PHistory checkHist(char *c)
{
    History *pNode;

    pNode=Hi.tail;
    char *ptr;
    long ret;
    
    ret = strtol(c, &ptr, 10);
    
#if DEBUG_MODE
    fprintf(stderr,"number %ld\n", ret);
    fprintf(stderr,"string |%s| \n", ptr);
#endif
    
    if(ret==0)
    {
        int counter=1;
        while(pNode != NULL  && counter<Hi.limit )
        {
            if(cmpStrings(pNode->command, c)){
                return pNode;
            }
            pNode = pNode->pPrev;
            counter=counter+1;
            
        }
        pNode=NULL;
        
    }
    else
    {
        
        int counter=1
        if(ret<0)//-1 for last command
            ret=ret*-1;
   
        if(ret <Hi.limit){
            while(pNode != NULL & counter<ret && counter<Hi.limit)
            {
#if DEBUG_MODE               
                fprintf(stderr,"%s  \n", pNode->command);
#endif
    
                pNode = pNode->pPrev;
                counter++;
            }
        }
        else
            pNode=NULL;
    }
    
    
    return pNode;
}
/* end of setup routine */
void executeChild(char *args[],int ct){
    
#if DEBUG_MODE
    fprintf(stderr,"I am child in executeChild %ld _%s\n", (long)getpid(),args[0]);
#endif
    
    if(cmpStrings(args[0],"/")==1){
      
       //this if-else block is completely unnecessary ,you should only  execvp system call
        if(ct<=7)
            execl(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],NULL);
        else
            execvp(args[0], &args[0]);
            perror("Failed to exec ");
    }
    else{
         execvp(args[0], &args[0]);
         perror("Failed to exec ");  
        
    }
    exit(1);//when the proccess faild to exec ,it forced to exit(kill ) on system
    
}
void AppendNodeH(History *hNode)
{
    
    
    if (hHead == NULL) {
        hHead = hNode;
        hNode->pPrev = NULL;
    }
    else {
        hTail->pNext = hNode;
        hNode->pPrev = hTail;
    }
    hTail = hNode;
    hNode->pNext = NULL;
    
}
void AppendNodeProcces(pid_t p)
{
    
    BProcces *pNode;
    pNode = (BProcces *)calloc(1,sizeof(BProcces));
    pNode->pid = p;
    pNode->alive=1;///default is alive
    
    
    
    if (pHead == NULL) {
        pNode->count=0;
        pHead = pNode;
        pNode->pPrev = NULL;
    }
    else {
        pTail->pNext = pNode;
        pNode->count=pTail->count+1;
        pNode->pPrev = pTail;
        
    }
    pTail = pNode;
    pNode->pNext = NULL;
    
}
void RemoveNodeProcces(BProcces *pNode)
{
    if (pNode->pPrev == NULL)
        pHead = pNode->pNext;
    else
        pNode->pPrev->pNext = pNode->pNext;
    if (pNode->pNext == NULL)
        pTail = pNode->pPrev;
    else
        pNode->pNext->pPrev = pNode->pPrev;
    free(pNode);
        
}

void DeleteAllHistory()
{
    while (pHead != NULL){
        RemoveNodeProcces(pHead);
    }
}

/*
 *
 *
 *WAİT ALL BACKGROUND PROCCES,İF NOT EXİST ,ERROR WAİT
 *
 *
 */

