#include "ros/ros.h"
#include <map>
#include <vector>
#include <string>
#include "sensor_msgs/JointState.h"
#include "trajectory_msgs/JointTrajectory.h"

// 回调函数，处理接收到的数据
void jointTrajectoryPointCallback(const trajectory_msgs::JointTrajectoryPoint::ConstPtr& msg)
{
    ros::NodeHandle nhCB;
    std::vector<double> joint_positions;
    std::vector<double> joint_velocities;
    std::vector<double> joint_accelerations;
    // 处理接收到的数据
    joint_positions = msg->positions; 
    joint_velocities = msg->velocities;
    joint_accelerations = msg->accelerations;
    // 控制函数


    // 将处理后的数据存储到ROS参数服务器,位置、速度、加速度信息
    nhCB.setParam("processed_positions", joint_positions);
    nhCB.setParam("processed_velocities", joint_velocities);
    nhCB.setParam("processed_accelerations", joint_accelerations);
    // ROS_INFO("Processed data stored in ROS parameter server: %f", processedData);
}

int main(int argc, char** argv)
{
    // 初始化ROS节点
    ros::init(argc, argv, "trajectory_point_handle");

    // 创建节点句柄
    ros::NodeHandle nh;

    // 创建一个订阅者，订阅指定话题
    ros::Subscriber sub = nh.subscribe("joint_trajectory_point_handle", 10, jointTrajectoryPointCallback);

    // 进入ROS循环
    ros::spin();

    return 0;
}