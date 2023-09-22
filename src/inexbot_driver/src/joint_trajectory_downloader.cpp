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


bool JointTrajectoryDownloader::send_to_robot(const std::vector<JointTrajPtFullMessage>& messages)
{
  // 创建 ROS 发布者，用于发布关节轨迹点消息   this->node_.
  // ros::NodeHandle nh_point;    
  // ros::Publisher joint_trajectory_point_pub = this->node_.advertise<trajectory_msgs::JointTrajectoryPoint>("joint_trajectory_point_handle", 10);
  // trajectory_msgs::JointTrajectoryPoint joint_traj_point_handled;

  bool rslt=true;
  std::vector<JointTrajPtFullMessage> points(messages);
  SimpleMessage msg;

  // std::vector<JointTrajPtFullMessage> points1;
  // std::vector<JointTrajPtFullMessage> points2;
  // std::vector<JointTrajPtFullMessage> points3(2);

  // 计算要分割的位置（中点）
  // size_t split_position = points.size() / 2;

  // 使用迭代器分割points并填充points1和points2
  // points1.assign(points.begin(), points.begin() + split_position -1);
  // points2.assign(points.begin() + split_position, points.end());
  // points3[0] = points[split_position];
  // points3[1] = points[points.size()];

  // timei_sub(points2);

  // Trajectory download requires at least two points (START/END)
  if (points.size() < 2)
    points.push_back(JointTrajPtFullMessage(points[0]));

  // The first and last points are assigned special sequence values
  // points.begin()->setSequence(SpecialSeqValues::START_TRAJECTORY_DOWNLOAD);
  // points.back().setSequence(SpecialSeqValues::END_TRAJECTORY);

  // points1.begin()->setSequence(SpecialSeqValues::START_TRAJECTORY_DOWNLOAD);
  // points1.back().setSequence(SpecialSeqValues::END_TRAJECTORY);
  // points2.begin()->setSequence(SpecialSeqValues::START_TRAJECTORY_DOWNLOAD);
  // points2.back().setSequence(SpecialSeqValues::END_TRAJECTORY);

  // points3.begin()->setSequence(SpecialSeqValues::START_TRAJECTORY_DOWNLOAD);
  // points3.back().setSequence(SpecialSeqValues::END_TRAJECTORY);

  if (!this->connection_->isConnected())
  {
    ROS_WARN("Attempting robot reconnection");
    this->connection_->makeConnect();
  }

  ROS_INFO("Sending trajectory points, size: %d", (int)points.size());


  for (int i = 0; i < (int)points.size()-1; ++i)
  {
    ROS_DEBUG("Sending joints trajectory point[%d]", i);

    // 从 points[i] 中提取位置、速度、加速度信息
    industrial::joint_data::JointData positions_1, positions_2,velocities, accelerations;
    std::vector<JointTrajPtFullMessage> points_11(11);
    points[i].point_.getPositions(positions_1);
    points[i+1].point_.getPositions(positions_2);
    // points[i].point_.getVelocities(velocities);
    // points[i].point_.getAccelerations(accelerations);

    // 将上面三个值转换为  industrial::joint_data::JointData 转换为 std::vector<double>
    // std::vector<double> joint_positions(6);
    // for (int j = 0; j < 6; ++j)
    // {
    //   double temp;
    //   temp = positions_1.getJoint(j);
    //   joint_positions[j] = temp;
    // }
    // std::vector<double> joint_velocities(6);
    // for (int k = 0; k < 6; ++k)
    // {
    //   double temp;
    //   temp = velocities.getJoint(k);
    //   joint_velocities[k] = temp;
    // }
    // std::vector<double> joint_accelerations(6);
    // for (int m = 0; m < 6; ++m)
    // {
    //   double temp;
    //   temp = accelerations.getJoint(m);
    //   joint_accelerations[m] = temp;
    // }

    // 创建 JointTrajectoryPoint 消息
    // trajectory_msgs::JointTrajectoryPoint joint_traj_point;
    // joint_traj_point.positions = joint_positions;
    // joint_traj_point.velocities = joint_velocities;
    // joint_traj_point.accelerations = joint_accelerations;

    // 发布 JointTrajectoryPoint 消息
    // joint_trajectory_point_pub.publish(joint_traj_point);
    // if (i > (int)points.size()-4)
    // {
    //   // ros::Duration delay(1);
    //   // delay.sleep(); 
    //   break;
    // }
    //接受参数服务器中处理过后的位置、速度、加速度信息
    // std::vector<double> joint_positions_handled;
    // std::vector<double> joint_velocities_handled;
    // std::vector<double> joint_accelerations_handled;
    // this->node_.getParam("processed_positions", joint_positions_handled);
    // this->node_.getParam("processed_velocities", joint_velocities_handled);
    // this->node_.getParam("processed_accelerations", joint_accelerations_handled);

    //更新points[i]
    // for (size_t i = 0; i < 6; ++i)
    // {
    //   double temp_p = joint_positions_handled[i];
    //   positions.setJoint(i, temp_p);

    //   double temp_v = joint_velocities_handled[i];
    //   velocities.setJoint(i, temp_v);

    //   double temp_a = joint_accelerations_handled[i];
    //   accelerations.setJoint(i, temp_a);
    // }

    // points[i].point_.setPositions(positions);
    // points[i].point_.setVelocities(velocities);
    // points[i].point_.setAccelerations(accelerations);

    // 将i点的位置分成十等份
    split_10_points_position(positions_1, positions_2, points_11);
    for (size_t i = 0; i < 11; i++)
    {
      rslt = send_split_points(points_11[i],points_11[i+1]);
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

  // ros::Duration(0.01).sleep();

  // for (int i = 0; i < (int)points2.size(); ++i)
  // {
  //   ROS_DEBUG("Sending joints trajectory point[%d]", i);

  //   points2[i].toTopic(msg);
  //   bool ptRslt = this->connection_->sendMsg(msg);
  //   if (ptRslt)
  //     ROS_DEBUG("Point[%d] sent to controller", i);
  //   else
  //     ROS_WARN("Failed sent joint point, skipping point");

  //   rslt &= ptRslt;// 按位与赋值
  // }


  return rslt;
  // return true;
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
  industrial::shared_types::shared_real time1 = 0.0;
  industrial::shared_types::shared_real time2 = 0.0;
  industrial::shared_types::shared_real time_sub = 0.0;
  point1.point_.getTime(time1);
  point2.point_.getTime(time2);
  time_sub = time2 - time1;
  point1.point_.setTime(0.0);
  point2.point_.setTime(0.1);
  // industrial::joint_data::JointData positions1, positions2, positions_sub;
  // points1.point_.getPositions(positions1);
  // points2.point_.getPositions(positions2);
  // positions_sub = positions2 - positions1;
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

