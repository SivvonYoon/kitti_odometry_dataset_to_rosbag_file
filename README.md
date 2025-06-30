# kitti odometry dataset to rosbag file
download here 👇 <br>
https://www.cvlibs.net/datasets/kitti/eval_odometry.php

<br>

## environment
ubuntu 18.04 <br>
ros melodic <br>

you have to catkin_make it!

<br>

### 1. git clone this project! (I already made source folder too. so it will be the catkin workspace itself.) 
<br>

### 2. rosrun
this is ex. for 10th data. <br>
```
rosrun mk_rosbag mk_rosbag \
    _cam0_timestamp_file:=${your workspace}/10/times.txt \
    _cam1_timestamp_file:=${your workspace}/10/times.txt \
    _cam0_folder:=${your workspace}/10/image_0/ \
    _cam1_folder:=${your workspace}/10/image_1/ \
    _output_bag_file:=kitti_10.bag

```
<br>

well done! ez right?

