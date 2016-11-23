#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "sys/wait.h"
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define DEBUG_MODE 0


struct History {
    struct History *pNext;
    struct History *pPrev;
    char command[100];
    char begining[10];
    
};
struct Hist {
    struct History * head;
    struct History * tail;
    int limit;
    //hist -set num
};
struct Hist Hi;
typedef struct History History;
History *hHead;
History *hTail;





int setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
    i,      /* loop index for accessing inputBuffer array */
    start,  /* index where beginning of next command parameter is */
    ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
    
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);
    
    /* 0 is the system predefined file descriptor for stdin (standard input),
     which is the user's screen in this case. inputBuffer by itself is the
     same as &inputBuffer[0], i.e. the starting address of where to store
     the command that is read, and length holds the number of characters
     read in. inputBuffer is not a null terminated C-string. */
    
    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
    
    /* the signal interrupted the read system call */
    /* if the process is in the read() system call, read returns -1
     However, if this occurs, errno is set to EINTR. We can check this  value
     and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }
    
    //printf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */
        
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
                break;
                
            default :             /* some other character */
                if (start == -1)
                    start = i;
                if (inputBuffer[i] == '&'){
                    *background  = 1;
                    inputBuffer[i-1] = '\0';
                    return ct+1;
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
int startsWith(const char *a, const char *b)
{
    if(strncmp(a, b, strlen(b)) == 0) return 1;
    return 0;
}
char* concatString(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}


const char *builtCommands[] = {"cd","dir","clr","wait","hist","exit","!"};
int startsWith(const char *a, const char *b);
void systemCommand(char *args[],int lastindex,int background,History *node);
void builtinCommand(char *args[],int index);
void executeChild(char *args[],int ct,History * node);
char *currentpath;
void AppendNodeH(History *hNode);
/*
void RemoveNodeH(History *hNode);
void DeleteAllHistory();
 */
void addHistory(int lastindex,char *args[]);

int main(void)
{
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /*command line arguments */
    Hi.limit=10;
   
    
  
    
    char cwd[1024];
    
    int flag=-1;
    
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        ///fprintf(stderr, "Current working dir: %s\n", cwd);
        
        currentpath=cwd;
    int lastindex=0;
    while (1){
        
        flag=-1;
        chdir(currentpath);
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            //fprintf(stderr, "-----------: %s\n", cwd);
            
            background = 0;
        fprintf(stderr,"CSE333sh: ");
        //printf("CSE333sh: ");
        /*setup() calls exit() when Control-D is entered */
        
        lastindex=setup(inputBuffer, args, &background);
        
        if(background==1)
            args[lastindex-1]=NULL;
        
        //addHistory(lastindex-background,args);
       
   
        
        /*
         if(strcmp(args[lastindex-1], "&")==0 | strcmp(args[lastindex-1], "|")==0){
         fprintf(stderr,"Last index background or pipe  %d",lastindex);
         }
         */
        /*
         char *envp[] =
         {
         "HOME=/home/student",
      1   "PATH=/bin:/usr/bin",
         "TZ=UTC0",
         "USER=student",
         "LOGNAME=tarzan",
         0
         };
         */
        int b;
        for(b=0;b<=6;b++)
        {
            if(strcmp(args[0], builtCommands[b])==0)//it is builtin command
            {
                flag=b;
                break;
            }
        }
        
        if(flag==-1)//it is shell command
            systemCommand(args, lastindex, background,NULL);
        else
            builtinCommand(args,flag);
        
        
        
        
        /** the steps are:
         (1) fork a child process using fork()
         (2) the child process will invoke execvp() or execl()
         (3) if background == 0, the parent will wait,
         otherwise it will invoke the setup() function again. */
    }
    
}
void addHistory(int lastindex,char *args[])
{
    //check if the bacground varsa 1 aşağidan başla
    char *c=NULL;;
    int i;
    char *result;;

    //bu raya ayar cek fatih
    if(startsWith(args[0],"/")==1)
    {
        if(lastindex==2)
            c=args[1];
        else if(lastindex>2)
        {
            for(i=1;(i+1)<lastindex;i++)
            {
                /*
                char *   temp=concatString(args[i]," ");
                c=concatString(c,temp);
                c=concatString(c,args[i+1]);
                 */

                
                char *temp=concatString(args[i]," ");
                c=temp;
                //  strcpy(c,temp);
                c=concatString(c,args[i+1]);
            }
        }
        else
        {
            //result=args[0];
        }
        
        
       result="";
        
    }
    else
    {
        if(lastindex==1)
            c=args[0];
        else
        {
            for(i=0;(i+1)<lastindex;i++)
            {
             char *temp=concatString(args[i]," ");
                c=temp;
              //  strcpy(c,temp);
                c=concatString(c,args[i+1]);
                
            }
        }
         result=concatString("/bin/",args[0]);
    }
    
    
    History *pNode;
    
    //pNode = (History *)malloc(sizeof(History));
    pNode = (History *)calloc(1,sizeof(History));

    //pNode->command = 1567+i;
    strncpy(pNode->command, c,strlen(c));
   // result=concatString("/bin/",args[0]);
    //strncpy(dest, src, 10);
    //strcpy(pNode->begining,temp);
    
    strncpy(pNode->begining, result, strlen(result));
    AppendNodeH(pNode);
    Hi.head=hHead;
    Hi.tail=hTail;

}
void systemCommand(char *args[],int lastindex,int background,History * node)
{
    ///fprintf(stderr,"I am  systemCommand %ld \n", (long)getpid());
    
    
    
    pid_t childpid;
    childpid = fork();
    
    if (childpid == -1) {
        perror("Failed to fork");
        //return 1;
    }
    if (childpid >0 ) {                          // parent code /
        
        if(!background)//background procces
        {    //waitpid(childpid,NULL,NULL);
            
            // if(wait(NULL>0);
            //wait(NULL);this  is equal waitpid(-1,NULL,0);
            waitpid(childpid, NULL, 0);
            /*
            
            if(childpid != wait(NULL)){
                perror(" Parent failed to waiting procces");
                
                
            }
             */
            
        }
        else{
            fprintf(stderr, "[i]  %ld \n", (long)childpid);
        }
        
    }
    if (childpid == 0) {// child code /
        //fprintf(stderr,"I am child proccess %ld \n", (long)getpid());
        
        executeChild(args,lastindex,node);
        // execvp(args[0], &args[0]);
        //int rc = strcmp(args[0], "cd");
        // fprintf(stderr," alo  %d",rc);
        //if(strcmp(args[0], "cd")!=0){
        
        
        //execve(args[0], &args[0], envp);
        
        /*
         execvp(args[0], &args[0]);
         perror("Child failed to execvp the command");
         */
        
        // }
        /*
         else{
         fprintf(stderr,"cd ye bastin: ");
         char c2;
         currentpath=args[1];
         fprintf(stdout, "Current working dir: %s\n", currentpath);
         }
         */
        
        
        //return 1;
    }
    
    /*
     else
     {
     perror(" failed to creating procces");
     return 1;
     }
     */
    /*
     if(strcmp(args[0], "cd")==0){
     if(startsWith(args[1],"/")==1)
     currentpath=args[1];
     
     }
     */
    
    // return 1;
}

void builtinCommand(char *args[],int index)
{
    
    
    
    // fprintf(stderr,"I am  builtinCommand %ld  index %d\n", (long)getpid(),index);
    if(index==0)//cd
    {
        
#if DEBUG_MODE
        int i;
        for (i = 0; i < strlen(args); i++)
            fprintf(stderr,"args %d = %s\n",i,args[i]);
#endif
        
        
         if(args[1]==NULL)
        {
            //strncpy(currentpath, getenv("HOME"),strlen(getenv("HOME")));
              strcpy(currentpath, getenv("HOME"));
        }
        else if(startsWith(args[1],"/")==1){
           // int a=strlen(args[1]);
            //strncpy(currentpath, args[1],a);
            strcpy(currentpath, args[1]);
            // currentpath=args[1];
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
       fprintf(stderr, " %s \n", currentpath);
    }
    else if(index==2)
    {
        system("clear");
    }
    else if(index==3)
    {
        pid_t ch_pid;
        while((ch_pid=wait(NULL)) > 0) { /* wait for all of your children */
            fprintf(stderr, "Done  ID:%ld\n",(long)ch_pid);
        }
        
    }
    else if(index==4)
    {
        History *pNode;
        for (pNode = Hi.head; pNode != NULL; pNode = pNode->pNext) {
            fprintf(stderr,"%s %s \n", pNode->begining,pNode->command);
            
        }

    }
    else if(index==6)
    {
        systemCommand(args,index,0,Hi.head);
    }
}
/* end of setup routine */
void executeChild(char *args[],int ct,History * node){
    // execvp(args[0], &args[0]);
    //int rc = strcmp(args[0], "cd");
    // fprintf(stderr," alo  %d",rc);
    //if(strcmp(args[0], "cd")!=0){
    
    
    //execve(args[0], &args[0], envp);
#if DEBUG_MODE
    fprintf(stderr,"I am child in executeChild %ld \n", (long)getpid());
#endif
    
    //execvp(args[0], &args[0]);
    //perror("Child failed to execvp the command");
    
    
    if(node==NULL){
        
        if(startsWith(args[0],"/")==0){
            
            execvp(args[0], &args[0]);
        }
        else{
            // execl(args[0],&args[0]);
            // execv(args[0],&args[1]);
            //background durumunda burda index kontrol yap
            int i;
            char *c;
            for(i=1;(i+1)<ct;i++)
            {
                c=concatString(args[i]," ");
                c=concatString(c,args[i+1]);
                
            }
            //  fprintf(stderr,"CSE333sh: ");
            execl(args[0], c, NULL);
            
        }
    }
    else
    {
        execl(node->begining, node->command, NULL);
    }
    
    /*
     int i;
     for (i = 0; i <= ct; i++)
     fprintf(stderr,"args %d = %s\n",i,args[i]);
     
     if(startsWith(args[0],"/")!=0)
     execvp(args[0], &args[0]);
     else{
     // execl(args[0],&args[0]);
     execv(args[0],&args[0]);
     }
     perror("Child failed to execvp the command");
     */
    //int execv(const char *path, char *const argv[]);
    // }
    /*
     else{
     fprintf(stderr,"cd ye bastin: ");
     char c2;
     currentpath=args[1];
     fprintf(stdout, "Current working dir: %s\n", currentpath);
     }
     */
    
    
    
    
    
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
/*
void DeleteAllHistory()
{
    while (hHead != NULL){
        RemoveNode(hHead);
    }
}

void RemoveNodeH(History *hNode)
{
    if (hNode->pPrev == NULL)
        hHead = hNode->pNext;
    else
        hNode->pPrev->pNext = hNode->pNext;
    if (hNode->pNext == NULL)
        hTail = hNode->pPrev;
    else
        hNode->pNext->pPrev = hNode->pPrev;
}

*/

















































/*
 *
 *
 *WAİT ALL BACKGROUND PROCCES,İF NOT EXİST ,ERROR WAİT
 *
 *
 */







