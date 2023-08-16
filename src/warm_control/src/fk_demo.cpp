#include <ros/ros.h>
#include <moveit/move_group_interface/move_group_interface.h>

int main(int argc, char **argv)
{
    ros::init(argc, argv, "fk_demo");
    ros::AsyncSpinner spinner(1);
    spinner.start();

    moveit::planning_interface::MoveGroupInterface arm("manipulator");

    arm.setGoalJointTolerance(0.001);  //单位是 m
    arm.setMaxAccelerationScalingFactor(0.2);
    arm.setMaxVelocityScalingFactor(0.2);
    //先控制机械臂回到初始化位置
    arm.setNamedTarget("home");
    arm.move();
    sleep(1);

    //设置6个关节的值，fk
    double targetPose[6] = {0.39, -0.67, -0.37, 0.0, 1.05, 0.45};
    std::vector<double> joint_group_position(6);
    joint_group_position[0] = targetPose[0];
    joint_group_position[1] = targetPose[1];
    joint_group_position[2] = targetPose[2];
    joint_group_position[3] = targetPose[3];
    joint_group_position[4] = targetPose[4];
    joint_group_position[5] = targetPose[5];

    arm.setJointValueTarget(joint_group_position);
    arm.move();
    sleep(1);

    //控制机械臂回到初始位置
    arm.setNamedTarget("home");
    arm.move();
    sleep(1);

    ros::shutdown();


    return 0;
}