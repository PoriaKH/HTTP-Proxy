#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libword.h"

int counter(char string[]){
   int i = 0;
   while(string[i] != NULL){
   i++;
   }
   return i;
}


char* int_to_str(int a){
   printf("a = %d\n",a);
   int r = 0;
   char* result = malloc(sizeof(char) * 50);   
   int number[50]; 

   int a_copy = a;
   while(a_copy > 0){
      printf("a_copy = %d\n",a_copy);
      number[r] = a_copy % 10;
      r++;
      a_copy /= 10;
   }
//
printf("r = %d\n",r);
for(int i = 0; i < r; i++){
printf("number[%d] = %d\n",i,number[i]);
}
//
   for(int i = r - 1; i >= 0; i--){
      printf("i = %d\n", i );
      if(number[i] == 0){
         strcat(result, "0");
      }
      else if(number[i] == 1)
         strcat(result, "1");
      else if(number[i] == 2)
         strcat(result, "2");
      else if(number[i] == 3)
         strcat(result, "3");
      else if(number[i] == 4)
         strcat(result, "4");
      else if(number[i] == 5)
         strcat(result, "5");
      else if(number[i] == 6)
         strcat(result, "6");
      else if(number[i] == 7)
         strcat(result, "7");
      else if(number[i] == 8)
         strcat(result, "8");
      else if(number[i] == 9)
         strcat(result, "9");
      else{
         printf("Error in int_to_str");
         return "Error in int_to_str";
      }
   }
   strcat(result, "\0");
   printf("result = %s\n",result);
   return result;
}

void slash_remover(char* string){
   int index = 0;
   for(int i = 0; ; i++){
      if(string[i] == '\0'){
         index = i;
         break;
      }
   }
   
   while(string[index - 1] == '/'){
      string[index - 1] = '\0';
      index--;
   }
   
}
