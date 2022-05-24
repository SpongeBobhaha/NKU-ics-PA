#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}


/////////////////////////////////////////////////

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
  uint64_t N=0;
//default step 1
  if(args==NULL)
       N=1;
  else{
       int nRet=sscanf(args,"%llu",&N);
       if(nRet<=0){
          printf("args error in cms_si\n");
          return 0;
       }
  }
  printf("in cmd_si,N=%llu\n",N);
  cpu_exec(N);
  return 0;

}
//print register
static int cmd_info(char *args) {
  char s;
  if(args==NULL){
      printf("args error in cmd_info\n");
      return 0;
  }
  int nRet=sscanf(args,"%c",&s);
  if(nRet<=0){
          printf("args error in cms_info\n");
          return 0;
   }
  if(s=='r'){
        int i;
        for(i=0;i<8;i++)
             printf("%s  0x%x\n",regsl[i],reg_l(i));
        printf("eip  0x%x\n",cpu.eip);
        for(i=0;i<8;i++)
             printf("%s  0x%x\n",regsw[i],reg_w(i));
        for(i=0;i<8;i++)
             printf("%s  0x%x\n",regsb[i],reg_b(i));
       printf("CR0=0x%x,CR3=0x%x\n",cpu.CR0,cpu.CR3); 
        return 0;
  }
  if(s=='w'){
        print_wp();
        return 0;
  }
  printf("args error in cms_info\n");
  return 0;
    
}

//scan memory

static int cmd_x(char *args) {
    
    int nLen=0;
    vaddr_t addr;
    char str[20];
    int nRet=sscanf(args,"%d %s",&nLen,str);
    if(nRet<=0){
        printf("args error in cms_si\n");
        return 0;
    }

//expr
     bool success;
     int res=expr(str,&success);
     addr=res;
     


     printf("Memory:");
     for (int i=0;i<nLen;i++){
         //every line 32bit
         if(i%4==0)
             printf( "\nx%x:  0x%02x",addr+i,vaddr_read(addr+i,1));
         else
             printf(" 0x%02x",vaddr_read(addr+i,1));
     }
      printf("\n");
      return 0;
}
     
static int cmd_p(char *args){
      bool success;
      int res=expr(args,&success);
      if(success==false)
           printf("error in expr()\n");
      else
           printf("the value of expr is:%d\n",res);
      return 0;
}

static int cmd_w(char* args){
      new_wp(args);
      return 0;
}

static int cmd_d(char* args){
      int num=0;
      int nRet=sscanf(args,"%d",&num);
      

      if(nRet<=0){
           printf("args error in cmd_si\n");
           return 0;
      }
      
      int r=free_wp(num);
      if(r==false)
          printf("error: no watchpoint %d\n",num);
      else
          printf("Success delete watchpoint %d\n",num);

      return 0;
}




//////////////////////////////////////////////////////////////////////////////


static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
//single step debugging
  {"si", "args:[N];execute [N] instructions step by step",cmd_si},
  
  {"info","args:r/w;print informations about registers or watchpoint",cmd_info},

  { "x", "x [N] [EXPR];scan the memory", cmd_x},
  { "p", "expr", cmd_p },

  { "w", "set the watchpoint", cmd_w },
  { "d", "delete the watchpoint", cmd_d },

  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
