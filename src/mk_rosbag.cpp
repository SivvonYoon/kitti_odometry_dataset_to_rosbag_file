#include <dirent.h>  // POSIX 디렉토리 함수
#include <algorithm> // std::sort
#include <ros/ros.h>
#include <rosbag/bag.h>
#include <sensor_msgs/Image.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

struct ImageInfo
{
    double timestamp;
    std::string file_path;
};

// 디렉토리에서 파일 이름 읽기 (C++11)
std::vector<std::string> getFilesInDirectory(const std::string &directory)
{
    std::vector<std::string> files;
    DIR *dir = opendir(directory.c_str());
    if (dir == nullptr)
    {
        ROS_ERROR("Failed to open directory: %s", directory.c_str());
        return files;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        // 파일 이름이 '.' 또는 '..'이 아닌 경우에만 추가
        if (entry->d_name[0] != '.')
        {
            files.push_back(entry->d_name);
        }
    }
    closedir(dir);

    // 파일 이름 정렬 (000000.png, 000001.png 순서 보장)
    std::sort(files.begin(), files.end());
    return files;
}

std::vector<ImageInfo> loadTimestampsAndImages(const std::string &timestamp_file, const std::string &image_folder)
{
    std::vector<ImageInfo> images;
    std::ifstream file(timestamp_file);
    if (!file.is_open())
    {
        ROS_ERROR("Failed to open timestamp file: %s", timestamp_file.c_str());
        return images;
    }

    std::vector<std::string> image_files = getFilesInDirectory(image_folder);
    if (image_files.empty())
    {
        ROS_ERROR("No images found in folder: %s", image_folder.c_str());
        return images;
    }

    double base_time = ros::Time::now().toSec(); // 기준 시간 설정

    std::string line;
    size_t image_index = 0;
    while (std::getline(file, line))
    {
        if (image_index >= image_files.size())
        {
            ROS_WARN("Not enough images in folder: %s", image_folder.c_str());
            break;
        }

        // 밀리초 단위를 초로 변환하고 기준 시간 추가
        double timestamp = base_time + std::stod(line) / 1e3;
        if (timestamp < 0.0)
        {
            ROS_WARN("Invalid timestamp: %f, skipping.", timestamp);
            continue;
        }

        std::string image_name = image_files[image_index++];
        images.push_back({timestamp, image_folder + "/" + image_name});
        ROS_INFO("Matched: %f %s", timestamp, image_name.c_str());
    }

    return images;
}

sensor_msgs::ImagePtr convertToRosImage(const ImageInfo &info)
{
    cv::Mat image = cv::imread(info.file_path, cv::IMREAD_GRAYSCALE);
    if (image.empty())
    {
        ROS_ERROR("Failed to read image: %s", info.file_path.c_str());
        return nullptr;
    }

    // 파일 이름에서 확장자 제거
    std::string file_name = info.file_path.substr(info.file_path.find_last_of("/") + 1); // 파일 이름 추출
    std::string seq_string = file_name.substr(0, file_name.find_last_of("."));           // 확장자 제거

    // ROS 메시지 생성
    sensor_msgs::ImagePtr msg = cv_bridge::CvImage(std_msgs::Header(), "mono8", image).toImageMsg();

    // Header에 seq 및 타임스탬프 설정
    msg->header.stamp = ros::Time(info.timestamp);
    msg->header.seq = std::stoi(seq_string);  // seq 필드에 정수로 변환하여 저장

    return msg;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "stereo_to_rosbag");

    // 입력 데이터 경로
    std::string cam0_timestamp_file, cam1_timestamp_file, cam0_folder, cam1_folder, output_bag_file;
    ros::NodeHandle nh("~");
    nh.getParam("cam0_timestamp_file", cam0_timestamp_file);
    nh.getParam("cam1_timestamp_file", cam1_timestamp_file);
    nh.getParam("cam0_folder", cam0_folder);
    nh.getParam("cam1_folder", cam1_folder);
    nh.getParam("output_bag_file", output_bag_file);

    // 이미지와 타임스탬프 로드
    std::vector<ImageInfo> cam0_images = loadTimestampsAndImages(cam0_timestamp_file, cam0_folder);
    std::vector<ImageInfo> cam1_images = loadTimestampsAndImages(cam1_timestamp_file, cam1_folder);

    ROS_WARN("Number of images in cam0 is (%ld)", cam0_images.size());

    if (cam0_images.size() != cam1_images.size())
    {
        ROS_ERROR("Number of images in cam0 (%ld) and cam1 (%ld) do not match!", cam0_images.size(), cam1_images.size());
        return -1;
    }

    rosbag::Bag bag;
    bag.open(output_bag_file, rosbag::bagmode::Write);

    for (size_t i = 0; i < cam0_images.size(); ++i)
    {
        ros::Time common_timestamp = ros::Time(cam0_images[i].timestamp);

        sensor_msgs::ImagePtr img0_msg = convertToRosImage(cam0_images[i]);
        sensor_msgs::ImagePtr img1_msg = convertToRosImage(cam1_images[i]);

        if (img0_msg)
            img0_msg->header.stamp = common_timestamp;

        if (img1_msg)
            img1_msg->header.stamp = common_timestamp;

        if (!img0_msg || !img1_msg)
        {
            ROS_WARN("Skipping unmatched images at index %ld", i);
            continue;
        }

        // ROS bag에 메시지 저장
        bag.write("/camera0/image_raw", common_timestamp, img0_msg);
        bag.write("/camera1/image_raw", common_timestamp, img1_msg);

        // 디버그 메시지에 seq 정보 추가
        ROS_INFO("Stored image0 with seq: %u, timestamp: %f", img0_msg->header.seq, common_timestamp.toSec());
        ROS_INFO("Stored image1 with seq: %u, timestamp: %f", img1_msg->header.seq, common_timestamp.toSec());
    }


    bag.close();
    ROS_INFO("ROS bag file created: %s", output_bag_file.c_str());
    return 0;
}
