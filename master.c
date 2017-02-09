#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>
#include <sys/epoll.h>

int main(int argc, char *argv[]){
	// argument number check
	if(argc<9){
		printf("Incorrect number of parameters entered - master - only %d given\n",argc);
		return 0;
	}
	double val, finalValue=0;
	int x,n,i,j,fd_count;
	x=atoi(argv[6]);
	n=atoi(argv[8]);
	pid_t pids[n+1];
	fd_set rfds;
	struct timeval tv;
	int fd[n+1][2];

	// mechanism validity check
	if(strcmp(argv[4],"sequential")!=0 && strcmp(argv[4],"select")!=0 && strcmp(argv[4],"poll")!=0 && strcmp(argv[4],"epoll")!=0){
		printf("Invalid mechanism entered.\n");
		return 0;
	}

	for(i=0;i<n+1;i++){
		// create pipe
		if(pipe(fd[i])<0){
			printf("Error creating pipe\n");
			return 0;
		}
		
		// create child process
		pids[i]=fork();
		// check forking error
		if(pids[i]==-1){
			printf("Forking error\n");
			return 0;
		}

		if(pids[i]==0){
			// In worker
			// calculate length of number corresponding to the worker
			int temp=i,n_str_size=0;
			while(temp){
				n_str_size++;
				temp/=10;
			}
			char *n_str;
			if(n_str_size==0){
				n_str=(char*)malloc(sizeof(char));
				n_str[0]='0';
			}else{
				n_str=(char*)malloc(n_str_size*sizeof(char));
				temp=i;
				int index=n_str_size-1;
				for(;index>=0;index--){
					n_str[index]=(char)('0'+(temp%10));
					temp/=10;
				}
			}
			close(STDOUT_FILENO);
			// check dup2 error
			if(dup2(fd[i][1],STDOUT_FILENO)==-1){
				printf("dup2 error\n");
			}
			close(fd[i][0]);
			close(fd[i][1]);
			char *arg[]={"./worker.o",argv[5],argv[6],argv[7],n_str,"-1",NULL};
			// run worker with proper path and argument list
			execvp(argv[2],arg);
			return 0;
		}
	}

	// close one end of pipe
	for(i=0;i<n+1;i++){
		close(fd[i][1]);
	}
	
	if(strcmp(argv[4],"sequential")==0){			// sequential
		//printf("in seq\n");
		for(j=0;j<n+1;j++){
			//printf("in loop %d\n",j);
			int returnStatus;
			waitpid(pids[j],&returnStatus,0);
			//printf("wait done %d\n",returnStatus);
			if(returnStatus==-1){
				printf("Error in wait!\n");
			}
			//printf("before read\n");
			//flushall();
			ssize_t size = read(fd[j][0],&val,sizeof(val));
			//printf("read done %ld\n",size);
			finalValue += val;
			printf("worker %d: %d^%d / %d! : %lf\n",j,x,j,j,val);
			close(fd[i][0]);
		}
	}else if(strcmp(argv[4],"select")==0){			// select
		int max_fd=fd[0][0];
		int fd_read[n+1];
		for(j=0;j<n+1;j++){
			fd_read[j]=0;
			FD_SET(fd[j][0],&rfds);
			if(fd[j][0]>max_fd){
				max_fd=fd[j][0];
			}
		}
		tv.tv_sec=3;
		tv.tv_usec=0;
		fd_count=0;
		int retval;
		while(fd_count<n+1){
			retval=select(max_fd+1,&rfds,NULL,NULL,&tv);
			if(retval==0){
				printf("Timeout\n");
			}else{
				for(j=0;j<n+1;j++){
					if(FD_ISSET(fd[j][0],&rfds)){
						ssize_t size = read(fd[j][0],&val,sizeof(val));
						finalValue += val;
						printf("worker %d: %d^%d / %d! : %lf\n",j,x,j,j,val);
						fd_read[j]=1;
						fd_count++;
						//close(fd[i][0]);
					}
				}
				FD_ZERO(&rfds);
				for(j=0;j<n+1;j++){
					if(fd_read[j]==0){
						FD_SET(fd[j][0],&rfds);
					}
				}
			}
		}
	}else if(strcmp(argv[4],"poll")==0){			// poll
		int fd_read[n+1];
		struct pollfd fds[n+1];
		for(j=0;j<n+1;j++){
			fd_read[j]=0;
			fds[j].fd = fd[j][0];
			fds[j].events = POLLIN;
			fds[j].revents=0;
		}
		fd_count=0;
		int retval;
		while(fd_count<n+1){
			retval=poll(fds,n+1,5);
			if(retval==0){
				printf("Timeout\n");
			}else if(retval==-1){
				printf("Error in poll\n");
			}else{
				for(j=0;j<n+1;j++){
					if((fds[j].revents & POLLIN) && (fd_read[j]==0)){
						ssize_t size = read(fd[j][0],&val,sizeof(val));
						finalValue += val;
						printf("worker %d: %d^%d / %d! : %lf\n",j,x,j,j,val);
						fd_read[j]=1;
						fd_count++;
						//close(fd[i][0]);
					}
				}
			}
		}
	}else if(strcmp(argv[4],"epoll")==0){			// epoll
		int epollfd;
		int fd_read[n+1];
		epollfd = epoll_create(n+1);
		struct epoll_event event[n+1];
		struct epoll_event *events;
		events=malloc(sizeof(struct epoll_event) * (n+1));
		for(j=0;j<n+1;j++){
			fd_read[j]=0;
			event[j].events = EPOLLIN;
			event[j].data.fd = fd[j][0];
			event[j].data.u32=j;
			if(epoll_ctl(epollfd,EPOLL_CTL_ADD,fd[j][0],event+j)==-1){
				printf("Error in epoll_ctl\n");
			}
		}
		fd_count=0;
		int retval;
		while(fd_count<n+1){
			retval=epoll_wait(epollfd,events,n+1,500);
			if(retval==0){
				printf("Error in epoll wait\n");
			}else if(retval >0){
				for(i=0;i<n+1;i++){
					for(j=0;j<retval;j++){
						if((fd_read[i]==0)){
							ssize_t size = read(fd[i][0],&val,sizeof(val));
							finalValue += val;
							printf("worker %d: %d^%d / %d! : %lf\n",i,x,i,i,val);
							fd_read[i]=1;
							fd_count++;
							break;
						}
					}
				}
			}
		}
	}
	printf("e^%d calculated till the %dth term is %lf\n",x,(n+1),finalValue);
	return 0;
}