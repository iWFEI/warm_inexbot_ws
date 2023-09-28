#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright 2019 Wuhan PS-Micro Technology Co., Itd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import rospy, sys
import moveit_commander
from moveit_commander import MoveGroupCommander
from geometry_msgs.msg import Pose, PoseStamped
from copy import deepcopy
from moveit_msgs.msg import RobotTrajectory
from trajectory_msgs.msg import JointTrajectoryPoint

import math
import numpy

def scale_trajectory_speed(traj, scale):
       # Create a new trajectory object
       new_traj = RobotTrajectory()
       
       # Initialize the new trajectory to be the same as the input trajectory
       new_traj.joint_trajectory = traj.joint_trajectory
       
       # Get the number of joints involved
       n_joints = len(traj.joint_trajectory.joint_names)
       
       # Get the number of points on the trajectory
       n_points = len(traj.joint_trajectory.points)
        
       # Store the trajectory points
       points = list(traj.joint_trajectory.points)
       
       # Cycle through all points and joints and scale the time from start,
       # as well as joint speed and acceleration
    #    n_points= n_points;
       print(n_points);
    #    print(range(n_points))
       for i in range(n_points):
           point = JointTrajectoryPoint()
           print(i)
           # The joint positions are not scaled so pull them out first
           point.positions = traj.joint_trajectory.points[i].positions

           # Next, scale the time_from_start for this point
           point.time_from_start = traj.joint_trajectory.points[i].time_from_start / scale
           
           # Get the joint velocities for this point
           point.velocities = list(traj.joint_trajectory.points[i].velocities)
           
           # Get the joint accelerations for this point
           point.accelerations = list(traj.joint_trajectory.points[i].accelerations)
         
           # Scale the velocity and acceleration for each joint at this point
           for j in range(n_joints):
               point.velocities[j] = point.velocities[j] * scale
               point.accelerations[j] = point.accelerations[j] * scale * scale
        
           # Store the scaled trajectory point
           points[i] = point

       # Assign the modified points to the new trajectory
       new_traj.joint_trajectory.points = points

       # Return the new trajecotry
       return new_traj

class MoveItCircleDemo:
    def __init__(self):
        # 初始化move_group的API
        moveit_commander.roscpp_initialize(sys.argv)

        # 初始化ROS节点
        rospy.init_node('moveit_clrcle_demo', anonymous=True)
                        
        # 初始化需要使用move group控制的机械臂中的arm group
        arm = MoveGroupCommander('manipulator')
        
        # 当运动规划失败后，允许重新规划
        arm.allow_replanning(True)
        
        # 设置目标位置所使用的参考坐标系
        reference_frame = 'base_link'
        arm.set_pose_reference_frame('base_link')
                
        # 设置位置(单位：米)和姿态（单位：弧度）的允许误差
        arm.set_goal_position_tolerance(0.01)
        arm.set_goal_orientation_tolerance(0.1)
        
        # 设置允许的最大速度和加速度
        arm.set_max_acceleration_scaling_factor(0.1)
        arm.set_max_velocity_scaling_factor(0.1)
        
        # 获取终端link的名称
        end_effector_link = arm.get_end_effector_link()

        # 控制机械臂先回到初始化位置
        arm.set_named_target('ready')
        arm.go()
        rospy.sleep(1)

        start_pose = arm.get_current_pose(end_effector_link).pose
                                               
        target_pose = Pose()
        target_pose.position.x = 0.0
        target_pose.position.y = -0.35
        target_pose.position.z = 0.35
        target_pose.orientation.x = 0
        target_pose.orientation.y = 0.7071
        target_pose.orientation.z = -0.7071
        target_pose.orientation.w = 0
        
        # 设置机械臂终端运动的目标位姿
        arm.set_pose_target(target_pose, end_effector_link)
        arm.go()

        # rospy.sleep(10)

        end_pose = deepcopy(target_pose)

        # 初始化路点列表
        waypoints = []
                
        # 将圆弧上的路径点加入列表

        centerA = target_pose.position.x
        centerB = target_pose.position.z
        radius = 0.1

        for th in numpy.arange(0, 6.28, 1.57):
            target_pose.position.x = centerA + radius * math.cos(th)
            target_pose.position.z = centerB + radius * math.sin(th)
            wpose = deepcopy(target_pose)
            waypoints.append(deepcopy(wpose))
            break


        fraction = 0.0   #路径规划覆盖率
        maxtries = 100   #最大尝试规划次数
        attempts = 0     #已经尝试规划次数
        
        # 设置机器臂当前的状态作为运动初始状态
        arm.set_start_state_to_current_state()
 
        # 尝试规划一条笛卡尔空间下的路径，依次通过所有路点，完成圆弧轨迹
        # (plan, fraction) = arm.compute_cartesian_path(
        #     waypoints,
        #     0.01,
        #     0.0,
        #     True)

        while fraction < 1.0 and attempts < maxtries:
            (plan, fraction) = arm.compute_cartesian_path (
                                    waypoints,   # waypoint poses，路点列表
                                    0.01,        # eef_step，终端步进值
                                    0.0,         # jump_threshold，跳跃阈值
                                    True)        # avoid_collisions，避障规划
            
            # 尝试次数累加
            attempts += 1
            
            # 打印运动规划进程
            if attempts % 10 == 0:
                rospy.loginfo("Still trying after " + str(attempts) + " attempts...")
                     
        # 如果路径规划成功（覆盖率100%）,则开始控制机械臂运动
        if fraction == 1.0:
            rospy.loginfo("Path computed successfully. Moving the arm.")
            # new_traj = scale_trajectory_speed(plan, 0.2)
            # arm.execute(new_traj)
            arm.execute(plan)
            rospy.loginfo("Path execution complete.")
        # 如果路径规划失败，则打印失败信息
        else:
            rospy.loginfo("Path planning failed with only " + str(fraction) + " success after " + str(maxtries) + " attempts.")  

        rospy.sleep(1)

        # 控制机械臂先回到初始化位置
        arm.set_named_target('ready')
        arm.go()
        rospy.sleep(1)
        
        # 关闭并退出moveit
        moveit_commander.roscpp_shutdown()
        rospy.spin()

if __name__ == "__main__":
    try:
        MoveItCircleDemo()
        # rospy.spin() 
    except rospy.ROSInterruptException:
        pass
