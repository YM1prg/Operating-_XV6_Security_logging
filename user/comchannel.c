#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {

int pids;
for (int i =0 ; i<3 ; i++){ 
pids = fork();
if (pids == 0 ){
printf("iam the parent");

}
else{



}

}
exit(0);

}
