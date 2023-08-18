search_mode=OPTIMIZE_MAX_JOINT
srdf_filename=warm.srdf
robot_name_in_srdf=warm
moveit_config_pkg=warm_moveit_config
robot_name=warm
planning_group_name=manipulator
ikfast_plugin_pkg=warm_moveit_ikfast_plugin
base_link_name=base_link
eef_link_name=wrist_3_link
ikfast_output_path=/home/neil/1_learn/warm_inexbot_ws/src/warm_moveit_ikfast_plugins/warm_moveit_ikfast_plugin/src/warm_manipulator_ikfast_solver.cpp

rosrun moveit_kinematics create_ikfast_moveit_plugin.py\
  --search_mode=$search_mode\
  --srdf_filename=$srdf_filename\
  --robot_name_in_srdf=$robot_name_in_srdf\
  --moveit_config_pkg=$moveit_config_pkg\
  $robot_name\
  $planning_group_name\
  $ikfast_plugin_pkg\
  $base_link_name\
  $eef_link_name\
  $ikfast_output_path
