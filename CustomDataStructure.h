#include <iostream>
#include <vector>
#include <stdexcept>

template <typename T>
class FixedSizeQueue {
public:
    // 构造函数，指定队列的最大容量
    FixedSizeQueue(size_t capacity) : capacity_(capacity), front_(0), size_(0) {
        queue_.resize(capacity);
    }
    FixedSizeQueue() {};
    // 添加元素到队列的末尾
    void push(const T& value) {
        if (size_ == capacity_) {
            throw std::out_of_range("Index out of range");
            return;
        }
        size_t index = (front_ + size_) % capacity_;
        queue_[index] = value;
        ++size_;
    }

    // 从队列的头部移除元素
    void pop() {
        if (isEmpty()) {
            throw std::out_of_range("IsEmpty");
            return;
        }
        front_ = (front_ + 1) % capacity_;
        --size_;
    }

    // 获取队列头部的元素
    T front() const {
        if (isEmpty()) {
            throw std::out_of_range("Queue is empty");
        }
        return queue_[front_];
    }

    // 检查队列是否为空
    bool isEmpty() const {
        return size_ == 0;
    }

    // 获取队列中的元素数量
    size_t size() const {
        return size_;
    }

    // 检查队列是否已满
    bool isFull() const {
        return size_ == capacity_;
    }

    // 通过索引访问队列中的元素
    T& operator[](size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return queue_[(front_ + index) % capacity_];
    }

    // 通过索引访问队列中的元素（只读）
    const T& operator[](size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return queue_[(front_ + index) % capacity_];
    }

private:
    std::vector<T> queue_;  // 存储队列元素的容器
    size_t capacity_;       // 队列的最大容量
    size_t front_;          // 队列头部的索引
    size_t size_;           // 当前队列中的元素数量
};

//#include<iostream>
//#include<vector>
//#include<opencv2/opencv.hpp>
//
//#include<cstdint>
//#include<fstream>
//
//using namespace cv;
//
////定义像素结构体
//typedef struct Pixel
//{
//	//每一个像素点的RGB通道值
//	uint8_t red;
//	uint8_t green;
//	uint8_t blue;
//
//	Pixel() :red(0), green(0), blue(0) {}
//	Pixel(uint8_t r, uint8_t g, uint8_t b) :red(r), green(g), blue(b) {}
//}Pixel;
//
//#pragma pack(push, 1) // 禁用结构体内存对齐以满足位图的格式规范
////定义位图文件头
//typedef struct BitmapFileHeader {
//	uint16_t file_type{ 0x4D42 }; // "BM" 字符
//	uint32_t file_size{ 0 };
//	uint16_t reserved1{ 0 };
//	uint16_t reserved2{ 0 };
//	uint32_t offset_data{ 0 };
//}BitmapFileHeader;
//
//
////信息头结构体
//typedef struct BitmapInfoHeader {
//	uint32_t size{ 0 };
//	int32_t width{ 0 };
//	int32_t height{ 0 };
//	uint16_t planes{ 1 };
//	uint16_t bit_count{ 0 };
//	uint32_t compression{ 0 };
//	uint32_t size_image{ 0 };
//	int32_t x_pixels_per_meter{ 0 };
//	int32_t y_pixels_per_meter{ 0 };
//	uint32_t colors_used{ 0 };
//	uint32_t colors_important{ 0 };
//}BitmapInfoHeader;
//#pragma pack(pop)
//
//
//
////定义位图类
//class Bitmap
//{
//public:
//	int width, height;					//the width and height of the picture which you want to dispose
//	std::vector<std::vector<Pixel>> pixels;		//the array which is used to save the information of the picture
//public:
//	Bitmap(int w, int h);
//	Bitmap(const cv::Mat& img);							 //transform ".jpg" type to Bitmap object
//	void setPixel(int x, int y, const Pixel& color);	//set the color of the picture inside the array 
//	Pixel getPixel(int x, int y) const;					//get the color in the specified positoin 
//
//	//定义位图写入功能
//	//这个函数接收一个 Bitmap 对象，并将其内容保存为 .bmp 文件：
//	void writeBitmapToFile(const std::string& file_path);
//
//};
//
//Bitmap::Bitmap(int w, int h)
//	:width(w), height(h), pixels(h, std::vector<Pixel>(w))
//{
//
//}
//
//Bitmap::Bitmap(const cv::Mat& img)
//	:width(img.cols), height(img.rows), pixels(img.cols, std::vector<Pixel>(img.rows))	//notice the initation of this vector
//{
//	for (int y = 0; y < img.rows; y++)
//	{
//		for (int x = 0; x < img.cols; x++)
//		{
//			//Mat的索引方式是[行索引][列索引]
//			auto color = img.at<cv::Vec3b>(y, x);
//			//std::cout << "Red: " << static_cast<int>(color[2]) << ", Green: " << static_cast<int>(color[1]) << ", Blue: " << static_cast<int>(color[0]) << std::endl;
//
//			// OpenCV 默认使用 BGR 格式
//			pixels[x][y] = Pixel(static_cast<int>(color[2]), static_cast<int>(color[1]), static_cast<int>(color[0]));
//		}
//	}
//}
//
//
//void Bitmap::setPixel(int x, int y, const Pixel& color)
//{
//	if (x >= 0 && x <= width && y >= 0 && y <= height)
//	{
//		pixels[x][y] = color;
//	}
//}
//
//Pixel Bitmap::getPixel(int x, int y)const
//{
//	if (x >= 0 && x <= width && y >= 0 && y <= height)
//	{
//		return pixels[x][y];
//	}
//	//如果坐标超出了图片的边界，则返回一个纯黑色的图片
//	return Pixel();
//}
//
//void Bitmap::writeBitmapToFile(const std::string& file_path)
//{
//	BitmapFileHeader file_header;		//文件头
//	BitmapInfoHeader info_header;		//信息头
//
//	//define the values of information header
//	info_header.size = sizeof(BitmapInfoHeader);
//	info_header.width = width;
//	info_header.height = height;
//	info_header.bit_count = 24; // 每个像素24位，即RGB
//	info_header.compression = 0; // 不压缩
//	info_header.size_image = width * height * 3; // 图像数据大小
//
//
//
//	file_header.file_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + info_header.size_image;
//	file_header.offset_data = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
//
//	std::ofstream file;
//	file.open(file_path, std::ios::out | std::ios::binary);
//	if (!file) {
//		std::cerr << "Unable to open file: " << file_path << std::endl;
//		return;
//	}
//
//	// 写入文件头和信息头
//	file.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
//	file.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));
//
//	// 写入像素数据
//	for (int y = 0; y < height; ++y)
//	{
//		for (int x = 0; x < width; ++x)
//		{
//			Pixel pixel = getPixel(x, y);
//			//cout << "A" << endl;
//			uint8_t color[3] = { pixel.blue, pixel.green, pixel.red };
//			//cout << "B" << endl;
//			//cout << color[0] << " " << color[1] << " " << color[2] << endl;
//			file.write(reinterpret_cast<const char*>(color), 3);
//		}
//	}
//
//	file.close();
//}
//
//
