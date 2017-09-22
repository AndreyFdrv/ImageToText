#include "opencv2/highgui/highgui.hpp"
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <iostream>
#include <math.h>
#include <fstream>
using namespace std;
const int clusters_count = 3;
struct rgb_color
{
	int r;
	int g;
	int b;
};
int **Cluster(IplImage *image)//кластеризация точек изображения методом k-means
{
	rgb_color *clusters_centers = new rgb_color[clusters_count];
	for (int i = 0; i < clusters_count; i++)
	{
		clusters_centers[i].r = rand() % 256;
		clusters_centers[i].g = rand() % 256;
		clusters_centers[i].b = rand() % 256;
	}
	int **clusters_numbers = new int*[image->height];
	for (int i = 0; i < image->height; i++)
		clusters_numbers[i] = new int[image->width];
	for (int y = 0; y<image->height; y++)
	{
		uchar* ptr = (uchar*)(image->imageData + y * image->widthStep);
		for (int x = 0; x<image->width; x++)
		{
			int b = ptr[3 * x];
			int g = ptr[3 * x + 1];
			int r = ptr[3 * x + 2];
			double min_distance = DBL_MAX;
			int cluster_number = 0;
			for (int i = 0; i < clusters_count; i++)
			{
				double distance = sqrt(pow(r - clusters_centers[i].r, 2) + pow(g - clusters_centers[i].g, 2) +
					pow(b - clusters_centers[i].b, 2));
				if (distance < min_distance)
				{
					min_distance = distance;
					cluster_number = i + 1;
				}
			}
			clusters_numbers[y][x] = cluster_number;
		}
	}
	rgb_color *old_clusters_centers = new rgb_color[clusters_count];
	while (true)
	{
		for (int i = 0; i < clusters_count; i++)
		{
			old_clusters_centers[i].r = clusters_centers[i].r;
			old_clusters_centers[i].g = clusters_centers[i].g;
			old_clusters_centers[i].b = clusters_centers[i].b;
		}
		for (int i = 0; i < clusters_count; i++)
		{
			clusters_centers[i].r = 0;
			clusters_centers[i].g = 0;
			clusters_centers[i].b = 0;
			for (int y = 0; y < image->height; y++)
			{
				uchar* ptr = (uchar*)(image->imageData + y * image->widthStep);
				for (int x = 0; x < image->width; x++)
				{
					if (clusters_numbers[y][x] == i + 1)
					{
						int b = ptr[3 * x];
						int g = ptr[3 * x + 1];
						int r = ptr[3 * x + 2];
						clusters_centers[i].r += r;
						clusters_centers[i].g += g;
						clusters_centers[i].b += b;
					}
				}
			}
			clusters_centers[i].r /= (image->width*image->height);
			clusters_centers[i].g /= (image->width*image->height);
			clusters_centers[i].b /= (image->width*image->height);
		}
		bool centers_changed = false;
		for (int i = 0; i<clusters_count; i++)
		{
			if (clusters_centers[i].r != old_clusters_centers[i].r)
				centers_changed = true;
			if (clusters_centers[i].g != old_clusters_centers[i].g)
				centers_changed = true;
			if (clusters_centers[i].b != old_clusters_centers[i].b)
				centers_changed = true;
		}
		if (!centers_changed)
			break;
		for (int y = 0; y<image->height; y++)
		{
			uchar* ptr = (uchar*)(image->imageData + y * image->widthStep);
			for (int x = 0; x<image->width; x++)
			{
				int b = ptr[3 * x];
				int g = ptr[3 * x + 1];
				int r = ptr[3 * x + 2];
				double min_distance = DBL_MAX;
				int cluster_number = 0;
				for (int i = 0; i < clusters_count; i++)
				{
					double distance = sqrt(pow(r - clusters_centers[i].r, 2) + pow(g - clusters_centers[i].g, 2) +
						pow(b - clusters_centers[i].b, 2));
					if (distance < min_distance)
					{
						min_distance = distance;
						cluster_number = i + 1;
					}
				}
				clusters_numbers[y][x] = cluster_number;
			}
		}
	}
	return clusters_numbers;
}
int ComputeNewClusterNumber(int **clusters_numbers, int x, int y, int step, float height_scale)
{
	int result = 0;
	for (int cluster_number = 1; cluster_number <= clusters_count; cluster_number++)
	{
		int max_count = 0;
		int count = 0;
		for (int i = height_scale*y*step; i < height_scale*(y+1)*step; i++)
		{
			for (int j = x*step; j < (x + 1)*step; j++)
			{
				if (clusters_numbers[i][j]==cluster_number)
					count++;
			}
		}
		if (count > max_count)
		{
			max_count = count;
			result = cluster_number;
		}
	}
	return result;
}
int **ReduceClustersNumbersArray(int **clusters_numbers, int &width, int &height, int max_size)
{
	float height_scale = 1.5;//у символов высота больше ширины, поэтому компенсируем это
	if ((width <= max_size) && (height <= max_size))
		return clusters_numbers;
	int step;
	if (width>height)
		step = width / max_size;
	else
		step = height / max_size;
	int new_width = width / step;
	int new_height = (height / step)/height_scale;
	int **new_clusters_numbers = new int*[new_height];
	for (int i = 0; i < new_height; i++)
		new_clusters_numbers[i] = new int[new_width];
	for (int y = 0; y < new_height; y++)
	{
		for (int x = 0; x < new_width; x++)
		{
			new_clusters_numbers[y][x] = ComputeNewClusterNumber(clusters_numbers, x, y, step, height_scale);
		}
	}
	for (int y = 0; y < height; y++)
	{
		delete[] clusters_numbers[y];
	}
	delete[] clusters_numbers;
	width = new_width;
	height = new_height;
	return new_clusters_numbers;
}
void CreateTextFile(IplImage *image, int **clusters_numbers, int width, int height, int max_size)
{
	clusters_numbers=ReduceClustersNumbersArray(clusters_numbers, width, height, max_size);
	ofstream result_file;
	result_file.open("result.txt");
	for (int y = 0; y<height; y++)
	{
		for (int x = 0; x<width; x++)
		{
			if (clusters_numbers[y][x] == 1)
				result_file << "|";
			else if (clusters_numbers[y][x] == 2)
				result_file << " ";
			else if (clusters_numbers[y][x] == 3)
				result_file << ".";
		}
		result_file << "\n";
	}
	result_file.close();
	std::cout << "Result has been saved in file result.txt\n";
}
int main()
{
	char filename[50];
	std::cout << "Input filename: ";
	cin.getline(filename, 50);
	IplImage* image = 0;
	image = cvLoadImage(filename, 1);
	if (image == 0)
	{
		std::cout << "File has not been found\n";
		std::system("pause");
		return 0;
	}
	int **clusters_numbers = Cluster(image);
	CreateTextFile(image, clusters_numbers, image->width, image->height, 50);
	system("pause");
	return 0;
}