
#include "std_msgs/String.h"  
#include<iostream>
#include <string>
#include <ros/ros.h>
#include <sensor_msgs/JointState.h>
#include <robot_state_publisher/robot_state_publisher.h>
#include <moveit/move_group_interface/move_group_interface.h>
//std_msgs/String存放在std_msgs package里，由String.msg文件自动生成的头文件
#include <control_msgs/FollowJointTrajectoryAction.h>  
#include <control_msgs/FollowJointTrajectoryActionGoal.h>
#include <control_msgs/FollowJointTrajectoryGoal.h>
#include <trajectory_msgs/JointTrajectory.h>
#include <trajectory_msgs/JointTrajectoryPoint.h>
#include <string>

#include <stdlib.h>
#include <stdio.h>
#include<sys/sem.h>
#include<sys/types.h>
#include<pthread.h>
#include<unistd.h>
#include<semaphore.h>
#include<vector>
#include<deque>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<sys/fcntl.h>
#define ARGS_CHECK(argc,n) { if( argc <n ) { printf("error/n");return 0;}}
#define ERROR_CHECK(argc,n,string) { if( argc ==n ) {perror(string);return 0;}}
#include<semaphore.h>
#include<vector>
#include<deque>
#include<sys/epoll.h>
using std::deque; 
using std::vector;

	typedef struct tData{
	double pos[6];
	double vel[6];
	double acc[6];
	int time;
	int n ; 
	}tData;
	typedef struct Joint_data{
	double pos[6];
	}Joint_data;
int socketFd;
tData data; 
deque<Joint_data> seq; 
int num ; 
sem_t mysem ; 
sem_t *sem; 
sem_t *sem_joint ; 
pthread_mutex_t mutex; 
pthread_mutex_t process ; 
pthread_mutexattr_t mutexattr; 



void  * joint_state ( void *args ) 
{
    struct epoll_event event,evs[1];
    int epfd=epoll_create(1);
    int ret ;
    event.events=EPOLLIN;
    event.data.fd=socketFd;
    ret=epoll_ctl(epfd,EPOLL_CTL_ADD,socketFd,&event);
    ERROR_CHECK(ret,-1,"epoll_ctl");

	printf("1\n");
	Joint_data joint; 
	

    while(1)
    {
	//printf("2\n");
  
        ret=epoll_wait(epfd,evs,1,-1);
       // printf("3\n");
	//printf("%d\n",ret);
        if(ret>0)
        {
                bzero(&joint,sizeof(joint));
                ret = recv ( socketFd, &joint , sizeof(joint) , 0 ) ;
		//printf("ret = %d \n",ret);
                if(ret<=0)
                {
                    printf("byebye\n");
                    break;
                }
		//printf("recv after\n");
		pthread_mutex_lock( &mutex) ; 
///printf("mutex\n");
		seq.push_back(joint) ; 
		num++;
		pthread_mutex_unlock(&mutex); 
//printf("sem\n");
		sem_post(sem);
//printf("seq size() %ld\n",seq.size());
                //printf("%f %f %f %f %f %f,ret = %d \n",joint.pos[0] ,seq[0].pos[1],joint.pos[2],joint.pos[3],joint.pos[4],joint.pos[5],ret );
       

        }
    }
    close(socketFd);	
}

//回调函数，当message topic消息到达后会调用此函数  control_msgs/FollowJointTrajectoryActionGoal
void execute(const  control_msgs::FollowJointTrajectoryActionGoal::ConstPtr& msg)
{  
	
std::cout<<"Traj"<<std::endl;
double a,b,c; 
int d ,e ; 
int ret ;
double time;
a=1;
tData data; 
        int n = msg->goal.trajectory.points.size();
	data.n = n ; 
	//ret = send(socketFd,&n,sizeof(n),0);
        //std::cout<<"int n " <<sizeof(msg->goal.trajectory.points)<<std::endl; 
    // std::cout<<"n="<<n<<std::endl;
	for ( int i= 0 ; i<n; i++)
	{
		
		//bzero(&data,sizeof(tData));

		for ( int  j = 0 ; j < 6 ; j++)
		{
			//std::cout<<msg->goal.trajectory.points[i].positions[j]<<std::endl;
		         data.pos[j]=msg->goal.trajectory.points[i].positions[j] *180/3.1415926;
 			
	 	         data.vel[j]=msg->goal.trajectory.points[i].velocities[j] *180/3.1415926;
	 	         data.acc[j]=msg->goal.trajectory.points[i].accelerations[j]*180/3.1415926;
	 	         //data.time[1]=msg->goal.trajectory.points[i].time_from_start.sec;
 			 
	 	         //data.time=msg->goal.trajectory.points[i].time_from_start.nsec;
	 	         //std::cout<<d<<std::endl ;
 			 //a=msg->goal.trajectory.points[i].positions[j] ;
	 	         //c=msg->goal.trajectory.points[i].accelerations[j];
	 	         //d=msg->goal.trajectory.points[i].time_from_start.sec;
	 	         //e=msg->goal.trajectory.points[i].time_from_start.nsec;

		}
		time=msg->goal.trajectory.points[i].time_from_start.toSec();
		data.time=time*1000;
//std::cout<<data.time<<std::endl;
 			ret = send(socketFd,&data,sizeof(data),0);
			if(ret ==-1)
{
perror("send");
}
//std::cout<<sizeof(data)<<std::endl;
			//std::cout<<"point"<<i<<"   "<<msg->goal.trajectory.points[i].positions[0] <<"   "
			 //   <<msg->goal.trajectory.points[i].velocities[0] <<"   "
			 //  <<msg->goal.trajectory.points[i].accelerations[0] <<std::endl;
        // std::cout<<i<<std::endl;
	}

	 

 
}  
  
  
int main(int argc, char **argv)  
{  
   //sem_init ( &mysem , 3, 0 ) ; 
    sem_unlink("my");
    sem = sem_open("my", O_CREAT, O_RDWR, 0);

//pthread_mutexattr_init(&mutexattr); 
//pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);

    //sem_unlink("joint");
    //sem_joint = sem_open("joint", O_CREAT, O_RDWR, 0);
   //sem_init( &sem_joint, 1, 0 ) ; 
   
    pthread_mutex_init(&process , &mutexattr); 
    socketFd=socket(AF_INET,SOCK_STREAM,0);
    ERROR_CHECK(socketFd,-1,"socket");
    struct sockaddr_in serAddr;
    bzero(&serAddr,sizeof(serAddr));
    serAddr.sin_family=AF_INET;
    serAddr.sin_port=htons(atoi("22"));
    serAddr.sin_addr.s_addr=inet_addr("192.168.1.13");
    int ret;
    ret=connect(socketFd,(struct sockaddr*)&serAddr,sizeof(serAddr));
    ERROR_CHECK(ret,-1,"connect");
    pthread_t pthid ,pthid1 ; 


    

 pid_t p = fork();
    if ( p !=0)
{
sleep(5);
printf("start\n");

ros::init(argc, argv, "Traj"); 
 ros::NodeHandle n;  

ros::Subscriber sub = n.subscribe("/joint_trajectory_action/goal", 10000, execute);  
   
    //ros::spin()进入自循环，可以尽可能快的调用消息回调函数  
    ros::spin(); 


}else{

sleep(3);
printf("second\n");
 pthread_mutex_init(&mutex, NULL) ; 
pthread_create(&pthid1 , NULL , joint_state , NULL) ; 


ros::init(argc, argv, "publisher"); 
    ros::AsyncSpinner spinner(1);
    spinner.start();
    moveit::planning_interface::MoveGroupInterface arm("manipulator");
    arm.setGoalJointTolerance(0.0001);
    arm.setMaxAccelerationScalingFactor(1);
    arm.setMaxVelocityScalingFactor(1);


        ros::NodeHandle n;

 //ros::Rate loop_rate(2);
	ros::Publisher joint_pub = n.advertise<sensor_msgs::JointState>("joint_states", 1);    
        std::vector<double> joint_group_positions(6);
        sensor_msgs::JointState joint_state;
        joint_state.name.resize(6);
        joint_state.position.resize(6);
	joint_state.name[0]="shoulder_pan_joint";
  
	joint_state.name[1] ="shoulder_lift_joint";

	joint_state.name[2] ="elbow_joint";

        joint_state.name[3] ="wrist_1_joint";

	joint_state.name[4] ="wrist_2_joint";

	joint_state.name[5] ="wrist_3_joint";

        Joint_data joint ; 






 while (ros::ok()) {

	bzero(&joint, sizeof(joint));

	sem_wait(sem)  ; 

	pthread_mutex_lock( &mutex) ; 
	joint= seq[0];
//printf("seq: %f , %f , %f , %f ,\n",seq[0].pos[0], seq[0].pos[1], seq[0].pos[2], seq[0].pos[3]);

	seq.pop_front(); 
	pthread_mutex_unlock(&mutex) ; 

        joint_state.header.stamp = ros::Time::now();
        joint_state.position[0] = joint.pos[0]*3.1415926/180;
        joint_state.position[1] = joint.pos[1]*3.1415926/180;
        joint_state.position[2] = joint.pos[2]*3.1415926/180;
        joint_state.position[3] =joint.pos[3]*3.1415926/180;
        joint_state.position[4] =joint.pos[4]*3.1415926/180;
        joint_state.position[5] = joint.pos[5]*3.1415926/180;

//printf("%f , %f , %f , %f\n",joint.pos[0],joint.pos[1],joint.pos[2],joint.pos[3]);
        joint_pub.publish(joint_state);


    
 

    }
}


    
   
    return 0;  
}  
