#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;
static int used_next;
static WP *wptemp;


void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
    wp_pool[i].oldValue=0;
    wp_pool[i].hitNum=0;
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
  used_next=0;
}


bool new_wp(char *args){

     if(free_ == NULL)
        assert(0);
     WP* result=free_;
     free_=free_->next;

     result->NO = used_next;
     used_next++;
     result->next=NULL;
     strcpy(result->e,args);
     result->hitNum=0;
   
     bool success;
     printf("%s",result->e);
     result->oldValue=expr(result->e,&success);
     
     if(success==false){
        printf("error in new_wp:expression fault!\n");
        return false;
     }
     
     wptemp=head;
     if(wptemp==NULL)
          head=result;
     else{
          while(wptemp->next!=NULL)
              wptemp=wptemp->next;
          wptemp->next=result;

    } 
     
    printf("Success: set watchpoint %d, oldValue=%d\n",result->NO,result->oldValue);
    return true;
/* TODO: Implement the functionality of watchpoint */

}
 
bool free_wp(int num){
     WP *thewp=NULL;
     if(head==NULL){
          printf("no watchpoint now\n");
          return false;
     }

     if(head->NO==num){
           thewp= head;
           head=head->next;
     }
     else{
           wptemp=head;
           while(wptemp!=NULL && wptemp->next!=NULL){
              if(wptemp->next->NO==num){
                   thewp=wptemp->next;
                   wptemp->next=wptemp->next->next;
                   break;
               }
               wptemp=wptemp->next;

           }
     }
      //add  num  in free_
     if(thewp!=NULL){
           thewp->next=free_;
           free_=thewp;
           return true;
     }
     return false;
}
           
void print_wp(){
          if(head==NULL){
              printf("no watchpoiny now\n");
              return;
           }

          printf("watchpoint:\n");
          printf("NO.    expr            hitTimes\n");
          wptemp=head;
          while(wptemp!=NULL){
               printf("%d     %s            %d\n",wptemp->NO,wptemp->e,wptemp->hitNum);                  
               wptemp=wptemp->next;
          }
}

// no change, return true.
bool watch_wp(){
        bool success;
        int result;
        if(head==NULL)
            return true;
 

        wptemp=head;
        while(wptemp!=NULL){
            result=expr(wptemp->e,&success);
            if(result!=wptemp->oldValue){
                 wptemp->hitNum+=1;
                 printf("Hardware watchpoint %d:%s\n",wptemp->NO,wptemp->e);
                 printf("Old value:%d\nNew value:%d\n\n",wptemp->oldValue,result);
                 wptemp->oldValue=result;
                 return false;
            }
            wptemp=wptemp->next;
         }
         return true;
}
         








           





   
