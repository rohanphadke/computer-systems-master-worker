#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>

double fact(int n){
	double denom=1.0;
	int i;
	for(i=1;i<=n;i++){
		denom*=i;
	}
	return denom;
}

int main(int argc, char *argv[]){
	int x,n,i;
	if(argc<5){
		printf("Enter correct number of parameters - worker - only %d given\n",argc);
		return 0;
	}
	x=atoi(argv[2]);
	n=atoi(argv[4]);
	if(argc==6){
		double val=pow(x,n)/fact(n);
		write(STDOUT_FILENO,&val,sizeof(val));
	}else{
		fprintf(stdout,"x^n / n! : %lf\n",pow(x,n)/fact(n));
	}
	return 0;
}