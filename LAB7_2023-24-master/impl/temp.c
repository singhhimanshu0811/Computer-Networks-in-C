#include <stdio.h>
#include <string.h>
#include<stdlib.h>

int main(){
    char temp[10]="rabbit";
    char* tem=temp+2;
    printf("%s", tem);
}
// int main() {
//   char input[] = "P:CSE=1;ECE=2;MECH=3;";
//   char *token;

//   // Skip the leading "P:" part
//   token = strtok(input, ":");
//   const char *delim="=;";
//     // Move to the first subject data
//   int x=1;
//   char name[5];
//   while (token != NULL) {
    
    
//     if(strcmp(token, "P")||strcmp(token, "G")|| strcmp(token, "H")){
//         if(x==1){
//             printf("%s~%s\n", token, name);
//             x=0;
//         }
//         else if(x==0){
//             strcpy(name, token);
//             x=1;
//         }
//     }
//     token=strtok(NULL, delim);
//   }

//   return 0;
// }
