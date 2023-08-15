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

} //joint_trajectory_downloader
} //industrial_robot_client

