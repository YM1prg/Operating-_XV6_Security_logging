#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main() {
int fds[2];
//int n;
int pids;

pipe(fds); 
pids = fork();
char *argv[] = {"hello","wow",0};
if (pids == 0 ){
//n = read(fds[0], buf, sizeof(buf)); 
exec("echo",argv);

}
else{


write(fds[1], "Iam parent", 10); 
wait(0);



}

}
