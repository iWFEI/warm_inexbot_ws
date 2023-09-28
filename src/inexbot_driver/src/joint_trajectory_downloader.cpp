/*
 * 20220915 
 */

#include "abb_driver/joint_trajectory_downloader.h"

namespace industrial_robot_client
{
namespace joint_trajectory_downloader
{

using industrial::simple_message::SimpleMessage;
namespace SpecialSeqValues = industrial::joint_traj_pt_full::SpecialSeqValues;

bool JointTrajectoryDownloader::send_to_robot1(const std::vector<JointTrajPtFullMessage>& messages)
{
  bool rslt=true;
  std::vector<JointTrajPtFullMessage> points(messages);
  SimpleMessage msg;

  // Trajectory download requires at least two points (START/END)
  if (points.size() < 2)
    points.push_back(JointTrajPtFullMessage(points[0]));

  // The first and last points are assigned special sequence values
  points.begin()->setSequence(SpecialSeqValues::START_TRAJECTORY_DOWNLOAD);
  points.back().setSequence(SpecialSeqValues::END_TRAJECTORY);

  if (!this->connection_->isConnected())
  {
    ROS_WARN("Attempting robot reconnection");
    this->connection_->makeConnect();
  }

  ROS_INFO("Sending trajectory points, size: %d", (int)points.size());

  for (int i = 0; i < (int)points.size(); ++i)
  {
    ROS_DEBUG("Sending joints trajectory point[%d]", i);

    points[i].toTopic(msg);
    bool ptRslt = this->connection_->sendMsg(msg);
    if (ptRslt)
      ROS_DEBUG("Point[%d] sent to controller", i);
    else
      ROS_WARN("Failed sent joint point, skipping point");

    rslt &= ptRslt;// 按位与赋值
  }

  return rslt;
}

bool JointTrajectoryDownloader::send_to_robot(const std::vector<JointTrajPtFullMessage>& messages)
{
  // 创建 ROS 发布者，用于发布关节轨迹点消息   this->node_.
  // ros::NodeHandle nh_point;    
  // ros::Publisher joint_trajectory_point_pub = this->node_.advertise<trajectory_msgs::JointTrajectoryPoint>("joint_trajectory_point_handle", 10);
  // trajectory_msgs::JointTrajectoryPoint joint_traj_point_handled;

  bool rslt=true;
  std::vector<JointTrajPtFullMessage> points(messages);
  SimpleMessage msg;

  // Trajectory download requires at least two points (START/END)
  if (points.size() < 2)
    points.push_back(JointTrajPtFullMessage(points[0]));

  if (!this->connection_->isConnected())
  {
    ROS_WARN("Attempting robot reconnection");
    this->connection_->makeConnect();
  }

  ROS_INFO("Sending trajectory points, size: %d", (int)points.size());
  // 延时flag
  bool delay_1_flag = false;
  bool delay_2_flag = false;
  int pointSize = (int)points.size()-1;
  for (int i = 0; i < pointSize; ++i)
  {
    ROS_DEBUG("Sending joints trajectory point[%d]", i);

    // 从 points[i] 中提取位置、速度、加速度信息
    industrial::joint_data::JointData positions_1, positions_2,velocities, accelerations;
    std::vector<JointTrajPtFullMessage> points_11(11);
    points[i].point_.getPositions(positions_1);
    points[i+1].point_.getPositions(positions_2);

    // 创建 JointTrajectoryPoint 消息
    // trajectory_msgs::JointTrajectoryPoint joint_traj_point;
    // 发布 JointTrajectoryPoint 消息
    // joint_trajectory_point_pub.publish(joint_traj_point);
    // if (i > (int)points.size()-4)
    // {
    //   // ros::Duration delay(1);
    //   // delay.sleep(); 
    //   break;
    // }

    if(i==0){
      delay_1_flag = true;
    }
    // 将i点的位置分成十等份
    split_10_points_position(positions_1, positions_2, points_11);
    for (size_t i = 0; i < 10; i++)
    {
      // ros::Duration(0.001).sleep(); 
      rslt = send_split_points(points_11[i],points_11[i+1]);
      if(i == 1){
        delay_2_flag = true;
      }
      if(delay_1_flag && delay_2_flag){
        ros::Duration(0.01).sleep(); 
      }

    }

    // rslt = split_send_points(points[i],points[i+1]);
    // ros::Duration delay(0.1);
    // delay.sleep(); 

    // points[i].toTopic(msg);
    // bool ptRslt = this->connection_->sendMsg(msg);
    // if (ptRslt)
    //   ROS_DEBUG("Point[%d] sent to controller", i);
    // else
    //   ROS_WARN("Failed sent joint point, skipping point");
    // rslt &= ptRslt;// 按位与赋值
  }

  return rslt;
}

bool JointTrajectoryDownloader::timei_sub(std::vector<JointTrajPtFullMessage>& points){
  industrial::shared_types::shared_real time0 = 0.0;
  industrial::shared_types::shared_real timei = 0.0;
  industrial::shared_types::shared_real time_sub = 0.0;
  points[0].point_.getTime(time0);
  for (size_t i = 0; i < (int)points.size(); i++)
  {
    points[i].point_.getTime(timei);
    time_sub = timei - time0;
    points[i].point_.setTime(time_sub);
  }

  return true;  
}


bool JointTrajectoryDownloader::send_split_points(JointTrajPtFullMessage point1, JointTrajPtFullMessage point2){
  SimpleMessage msg;
  point1.point_.setTime(0.0);
  point2.point_.setTime(0.01);
  point1.setSequence(SpecialSeqValues::START_TRAJECTORY_DOWNLOAD);
  point2.setSequence(SpecialSeqValues::END_TRAJECTORY);

  point1.toTopic(msg);
  bool ptRslt = this->connection_->sendMsg(msg);

  point2.toTopic(msg);
  ptRslt = this->connection_->sendMsg(msg);

  return true;
}

bool JointTrajectoryDownloader::split_10_points_position(industrial::joint_data::JointData positions_1, industrial::joint_data::JointData positions_2,
                                                std::vector<JointTrajPtFullMessage>& points_11
                                                )
{
  std::vector<double> joint_positions_1(6);
  std::vector<double> joint_positions_2(6);
  std::vector<double> joint_positions_div_10(6);
  industrial::joint_data::JointData positions_temp;
  for (int j = 0; j < 6; ++j)
  {
    double temp, temp_sub_div_10;
    temp = positions_1.getJoint(j);
    joint_positions_1[j] = temp;

    temp = positions_2.getJoint(j);
    joint_positions_2[j] = temp;

    temp_sub_div_10 = (joint_positions_2[j] - joint_positions_1[j])/10;
    joint_positions_div_10[j] = temp_sub_div_10;
  }
  for (size_t j = 0; j < 11; j++)
  {
    for (size_t i = 0; i < 6; i++)
    {
      positions_temp.setJoint(i, joint_positions_1[i] + joint_positions_div_10[i]*j);
    }
    points_11[j].point_.setPositions(positions_temp);
  }
  
  return true;
}

} //joint_trajectory_downloader
} //industrial_robot_client

