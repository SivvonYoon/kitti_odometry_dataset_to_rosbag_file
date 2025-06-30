cmake_minimum_required(VERSION 2.8.3)
project(mk_rosbag)

# 빌드 설정
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -g")

# ROS 패키지 찾기
find_package(catkin REQUIRED COMPONENTS
  roscpp
  rosbag
  cv_bridge
  sensor_msgs
  std_msgs
  image_transport
)

# OpenCV 패키지 찾기
find_package(OpenCV REQUIRED)

# catkin 설정
catkin_package()

# include 디렉토리 설정
include_directories(
  ${catkin_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)

# 실행 파일 생성
add_executable(mk_rosbag mk_rosbag.cpp)

# 링크 라이브러리 설정
target_link_libraries(mk_rosbag
  ${catkin_LIBRARIES}
  ${OpenCV_LIBRARIES}
)
