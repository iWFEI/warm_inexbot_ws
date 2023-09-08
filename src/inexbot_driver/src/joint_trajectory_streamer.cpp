/*
 * Software License Agreement (BSD License)
 *
 * Copyright (c) 2011, Southwest Research Institute
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 	* Redistributions of source code must retain the above copyright
 * 	notice, this list of conditions and the following disclaimer.
 * 	* Redistributions in binary form must reproduce the above copyright
 * 	notice, this list of conditions and the following disclaimer in the
 * 	documentation and/or other materials provided with the distribution.
 * 	* Neither the name of the Southwest Research Institute, nor the names
 *	of its contributors may be used to endorse or promote products derived
 *	from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "abb_driver/joint_trajectory_streamer.h"

using industrial::simple_message::SimpleMessage;

namespace industrial_robot_client
{
namespace joint_trajectory_streamer
{

// bool JointTrajectoryStreamer::init(SmplMsgConnection* connection, const std::vector<std::string> &joint_names,
//                                    const std::map<std::string, double> &velocity_limits)
bool JointTrajectoryStreamer::init(std::string default_ip, int default_port)
{
  bool rtn = true;

  ROS_INFO("JointTrajectoryStreamer: init");

  this->mutex_.lock();
  this->current_point_ = 0;
  this->state_ = TransferStates1::IDLE;
  this->streaming_thread_ =
      new boost::thread(boost::bind(&JointTrajectoryStreamer::streamingThread, this));
  ROS_INFO("Unlocking mutex");
  this->mutex_.unlock();

  rtn &= JointTrajectoryInterface::init(default_ip, default_port);

  return rtn;
}

JointTrajectoryStreamer::~JointTrajectoryStreamer()
{
  delete this->streaming_thread_;
}

void JointTrajectoryStreamer::jointTrajectoryCB(const trajectory_msgs::JointTrajectoryConstPtr &msg)
{
  ROS_INFO("Receiving joint trajectory message");

  // read current state value (should be atomic)
  const auto state = this->state_;

  ROS_DEBUG("Current state is: %d", state);

  // always request a stop of current trajectory execution if an empty trajectory
  // is received. We handle this separately from the check below, as the server
  // might be executing a trajectory which this client has already finished
  // uploading (due to buffering on the server side fi), and then our local state
  // would be "IDLE", and we'd end up not sending the stop request.
  if (msg->points.empty())
  {
    ROS_INFO_STREAM("Empty trajectory received while in state: " << TransferStates1::to_string1(state) << ". Canceling current trajectory.");
    this->mutex_.lock();
    trajectoryStop();
    this->mutex_.unlock();
    return;
  }

  // if we're currently streaming a trajectory and we're requested to stream another
  // we complain, as splicing is not supported. Cancellation of the current trajectory
  // should first be requested, then a new trajectory started.
  if (TransferStates1::IDLE != state)
  {
    ROS_ERROR("Trajectory splicing not yet implemented, stopping current motion.");
    this->mutex_.lock();
    trajectoryStop();
    this->mutex_.unlock();
    return;
  }

  // calc new trajectory
  std::vector<JointTrajPtFullMessage> new_traj_msgs;
  if (!trajectory_to_msgs(msg, &new_traj_msgs))
    return;

  // send command messages to robot
  send_to_robot(new_traj_msgs);
}

bool JointTrajectoryStreamer::send_to_robot(const std::vector<JointTrajPtFullMessage>& messages)
{
  ROS_INFO("Loading trajectory, setting state to streaming");
  this->mutex_.lock();
  {
    ROS_INFO("Executing trajectory of size: %d", (int)messages.size());
    this->current_traj_ = messages;
    this->current_point_ = 0;
    this->state_ = TransferStates1::STREAMING;
    this->streaming_start_ = ros::Time::now();
  }
  this->mutex_.unlock();

  return true;
}

bool JointTrajectoryStreamer::trajectory_to_msgs(const trajectory_msgs::JointTrajectoryConstPtr &traj, std::vector<JointTrajPtFullMessage>* msgs)
{
  // use base function to transform points
  if (!JointTrajectoryInterface::trajectory_to_msgs(traj, msgs))
    return false;

  // pad trajectory as required for minimum streaming buffer size
  if (!msgs->empty() && (msgs->size() < (size_t)min_buffer_size_))
  {
    ROS_DEBUG("Padding trajectory: current(%d) => minimum(%d)", (int)msgs->size(), min_buffer_size_);
    while (msgs->size() < (size_t)min_buffer_size_)
      msgs->push_back(msgs->back());
  }

  return true;
}


void JointTrajectoryStreamer::streamingThread()
{
  JointTrajPtFullMessage jtpMsg;
  int connectRetryCount = 1;

  ROS_INFO("Starting joint trajectory streamer thread");
  while (ros::ok())
  {
    ros::Duration(0.005).sleep();

    // automatically re-establish connection, if required
    if (connectRetryCount-- > 0)
    {
      ROS_INFO("Connecting to robot motion server");
      this->connection_->makeConnect();
      ros::Duration(0.250).sleep();  // wait for connection

      if (this->connection_->isConnected())
        connectRetryCount = 0;
      else if (connectRetryCount <= 0)
      {
        ROS_ERROR("Timeout connecting to robot controller.  Send new motion command to retry.");
        this->state_ = TransferStates1::IDLE;
      }
      continue;
    }

    this->mutex_.lock();

    SimpleMessage msg, reply;
        
    switch (this->state_)
    {
      case TransferStates1::IDLE:
        ros::Duration(0.010).sleep();  //  loop while waiting for new trajectory
        break;

      case TransferStates1::STREAMING:
        if (this->current_point_ >= (int)this->current_traj_.size())
        {
          ROS_INFO("Trajectory streaming complete, setting state to IDLE");
          this->state_ = TransferStates1::IDLE;
          break;
        }

        if (!this->connection_->isConnected())
        {
          ROS_DEBUG("Robot disconnected.  Attempting reconnect...");
          connectRetryCount = 5;
          break;
        }

        jtpMsg = this->current_traj_[this->current_point_];
        jtpMsg.toRequest(msg);
            
        ROS_DEBUG("Sending joint trajectory point");
        if (this->connection_->sendAndReceiveMsg(msg, reply, false))
        {
          this->current_point_++;
          ROS_INFO("Point[%d of %d] sent to controller",
                   this->current_point_, (int)this->current_traj_.size());
        }
        else
          ROS_WARN("Failed sent joint point, will try again");

        break;
      default:
        ROS_ERROR("Joint trajectory streamer: unknown state");
        this->state_ = TransferStates1::IDLE;
        break;
    }

    this->mutex_.unlock();
  }

  ROS_WARN("Exiting trajectory streamer thread");
}

void JointTrajectoryStreamer::trajectoryStop()
{
  JointTrajectoryInterface::trajectoryStop();

  ROS_DEBUG("Stop command sent, entering idle mode");
  this->state_ = TransferStates1::IDLE;
}

} //joint_trajectory_streamer
} //industrial_robot_client
