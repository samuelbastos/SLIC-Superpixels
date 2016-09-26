/*
* File:   main.cpp
* Author: jcoelho
*
* Created on September 11, 2016, 2:18 PM
*/

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
#include "Image.h"
#include "Cluster.h"

using namespace std;


/**
* TODO: converte uma imagem rgb em uma imagem Lab.
* @param rgb - imagem rgb de entrada.
* @return - imagem em Lab.
*/
Image convertImageFromRGB2Lab(const Image& rgb)
{
	Image Lab(rgb);

	//Converte cada pixel da imagem para XYZ
	for (int i = 0; i < rgb.getW(); i++)
	{
		for (int j = 0; j < rgb.getH(); j++)
		{
			Pixel convertedPixel = rgb.rgbToXYZ(rgb.getPixel(i, j));

			Lab.setPixel(i,j,convertedPixel);
		}
	}

	// Converte cada pixel da imagem para LAB
	for (int i = 0; i < rgb.getW(); i++)
	{
		for (int j = 0; j < rgb.getH(); j++)
		{
			Pixel convertedPixel = Lab.XYZToLab(Lab.getPixel(i, j));

			Lab.setPixel(i,j, convertedPixel);
		}
	}

	return Lab;
}



/**
* TODO: converte uma imagem Lab em uma imagem em rgb.
* @param Lab - imagem Lab de entrada.
* @return - imagem em RGB.
*/
Image convertImageFromLAB2RGB(const Image& Lab)
{
	Image rgb(Lab);

	//Converte cada pixel da imagem para XYZ
	for (int i = 0; i < Lab.getW(); i++)
	{
		for (int j = 0; j < Lab.getH(); j++)
		{
			Pixel convertedPixel = Lab.LabToXYZ(Lab.getPixel(i, j));

			rgb.setPixel(i,j,convertedPixel);
		}
	}

	// Converte cada pixel da imagem para LAB
	for (int ii = 0; ii < Lab.getW(); ii++)
	{
		for (int jj = 0; jj < Lab.getH(); jj++)
		{
			Pixel convertedPixel = rgb.XYZTorgb(rgb.getPixel(ii, jj));

			rgb.setPixel(ii,jj, convertedPixel);
		}
	}

	return rgb;
}

void calculateLocalMinimum(Image& Lab, Pixel center, float clusterCenterX, float clusterCenterY, float& minimumX, float& minimumY)
{
	double minGradient = 1000000000000000;

	for (float i = clusterCenterX - 1; i < clusterCenterX + 2; i++)
	{
		for (float j = clusterCenterY - 1; j < clusterCenterY + 2; j++)
		{
			Pixel c1 = Lab.getPixel((int)i, (int)j+1);
			Pixel c2 = Lab.getPixel((int)i	+1, (int)j);
			Pixel c3 = Lab.getPixel((int)i, (int)j);
			
			double i1 = c1[0];
			double i2 = c2[0];
			double i3 = c3[0];

			double gradient = sqrt(pow((i1 - i3), 2)) + sqrt(pow(i2 - i3, 2));

			if (gradient < minGradient)
			{
				minGradient = fabs(i1 - i3) + fabs(i2 - i3);

				minimumX = i;
				minimumY = j;
			}

		}
	}
}

/**
* TODO: inicializa os clusters.
* @param clusters - clusters que DEVEM ser alocados e inicializados.
* @param Lab - imagem em Lab.
* @param k - numero desejado de superpixels.
* @return - numero de superpixels.
*/
int initializeClusters(Cluster*& clusters, Image& Lab, int k, double s)
{	
	clusters = new Cluster[k];
	int step = int(s);
	int clusterCounter = 0;
	float minX = 0.0;
	float minY = 0.0;

	for (int i = step; i < Lab.getW() - 1; i += step)
	{
		for (int j = step; j < Lab.getH() - 1; j += step)
		{
				calculateLocalMinimum(Lab, Lab.getPixel(i, j), (float)i, (float)j, minX, minY);
				clusters[clusterCounter] = Cluster(Lab.getPixel((int)minX,(int)minY), minX, minY);
				clusterCounter++;			
		}
	}
	
	return clusterCounter;
}


double computeDistance(Pixel clusterCenter, float clusterX, float clusterY, Pixel i, float iX, float iY, double ms, double mc)
{
	double dc = sqrt(pow((clusterCenter[0] - i[0]), 2) + pow((clusterCenter[1] - i[1]), 2) + pow((clusterCenter[2] - i[2]), 2));
	double ds = sqrt(pow((clusterX - iX), 2) + pow((clusterY - iY), 2));

	double dt = sqrt(pow((dc / mc), 2) + pow((ds / ms), 2));
	
	return dt;
}

/**
* TODO: realiza o algoritmo de superpixels.
* @param Lab - Imagem em lab.
* @param clusters - clusters inicializados.
* @param labels - labels inicializadas.
* @param k - numero de superpixels.
* @param M - compacidade.
*/
void performSuperPixelsAlgorithm(Image& Lab, Cluster* clusters, int*& labels, int k, double M, double s)
{
	int n = Lab.getH()*Lab.getW();
	std::vector<double> distance; 
	std::vector<Pixel>originalColours;
	std::vector<int> centerCount;
	int step = int(s);

	int iter = 0;
	
	for (int j = 0; j < Lab.getW(); j++)
	{
		for (int k = 0; k < Lab.getH(); k++)
		{
			distance.push_back(10000000000.0);
			labels[Lab.computePosition(j, k)] = -1;
		}
	}


	//Enquanto erro for grande e o número de iterações pequeno, continuar
	while (iter < 10)
	{	
		//Para cada superpixel j
		for (int j = 0; j < k; j++)
		{	
			originalColours.push_back(clusters[j].getPixel());
			 
			//Para cada pixel dentro de um range de 2Sx2S
			for (int z = (int)(clusters[j].getX() - s); z < int(clusters[j].getX() + s); z++)
			{
				for (int w = (int)(clusters[j].getY() - s); w < int(clusters[j].getY() + s); w++)
				{
					if (z >= 0 && z < Lab.getW() && w >= 0 && w < Lab.getH())
					{
						double D = computeDistance(clusters[j].getPixel(), clusters[j].getX(), clusters[j].getY(), Lab.getPixel(z, w), (float)z, (float)w, s, M);
						int pixelIndex = Lab.computePosition(z, w);

						if (D < distance[pixelIndex])
						{
							distance[pixelIndex] = D;
							labels[pixelIndex] = j;
						}
					}
						
				}
			}
	
		}

	
		
		//Clear nos clusters
		Pixel clearPixel = Pixel();
		for (int j = 0; j < k; j++)
		{
			clusters[j].set(originalColours[j], 0, 0);
			centerCount.push_back(0);
		}
		

		//Recalcula novos centros de cluster (média dos pixels de cada cluster)
		for (int z = 0; z < Lab.getW(); z++)
		{
			for (int w = 0; w < Lab.getH(); w++)
			{
				int pixelClusterId = labels[Lab.computePosition(z, w)];

				if (pixelClusterId != -1)
				{
					clusters[pixelClusterId].set(originalColours[pixelClusterId], (float)z, (float)w);

					clusters[pixelClusterId].setPosition(clusters[pixelClusterId].getX() + z, clusters[pixelClusterId].getY() + w);
					centerCount[pixelClusterId] += 1;
				}
			}
		}
		
		for (int j = 0; j < k; j++)
		{
			clusters[j].setPosition(clusters[j].getX() / centerCount[j], clusters[j].getY() / centerCount[j]);
		}

		iter++;
	}	
	
}



void drawContoursAroundSegments(Image& rgb, int* labels, Pixel borderColor = Pixel())
{
	int w = rgb.getW();
	int h = rgb.getH();

	const int dx8[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
	const int dy8[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };

	int sz = w * h;
	vector<bool> istaken(sz, false);
	vector<int> contourx(sz);
	vector<int> contoury(sz);
	int mainindex(0);
	int cind(0);
	for (int j = 0; j < h; j++)
	{
		for (int k = 0; k < w; k++)
		{
			int np(0);
			for (int i = 0; i < 8; i++)
			{
				int x = k + dx8[i];
				int y = j + dy8[i];

				if ((x >= 0 && x < w) && (y >= 0 && y < h))
				{
					int index = y * w + x;

					if (false == istaken[index])//comment this to obtain internal contours
					{
						if (labels[mainindex] != labels[index]) np++;
					}
				}
			}
			if (np > 1)
			{
				contourx[cind] = k;
				contoury[cind] = j;
				istaken[mainindex] = true;
				//img[mainindex] = color;
				cind++;
			}
			mainindex++;
		}
	}

	int numboundpix = cind; //int(contourx.size());
	for (int j = 0; j < numboundpix; j++)
	{
		for (int n = 0; n < 8; n++)
		{
			int x = contourx[j] + dx8[n];
			int y = contoury[j] + dy8[n];
			if ((x >= 0 && x < w) && (y >= 0 && y < h))
			{
				int ind = rgb.computePosition(x, y);
				if (!istaken[ind])
				{
					rgb.setPixel(ind, borderColor);
				}
			}
		}
	}
}



void enforceLabelConnectivity(const int* labels, //input labels that need to be corrected to remove stray labels
	const int width,
	const int height,
	int*& nlabels, //new labels
	int& numlabels, //the number of labels changes in the end if segments are removed
	const int& K) //the number of superpixels desired by the user
{
	const int dx4[4] = { -1, 0, 1, 0 };
	const int dy4[4] = { 0, -1, 0, 1 };

	const int sz = width * height;
	const int SUPSZ = sz / K;

	for (int i = 0; i < sz; i++) nlabels[i] = -1;
	int label(0);
	int* xvec = new int[sz];
	int* yvec = new int[sz];
	int oindex(0);
	int adjlabel(0); //adjacent label
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (0 > nlabels[oindex])
			{
				nlabels[oindex] = label;
				//--------------------
				// Start a new segment
				//--------------------
				xvec[0] = k;
				yvec[0] = j;
				//-------------------------------------------------------
				// Quickly find an adjacent label for use later if needed
				//-------------------------------------------------------
				{
					for (int n = 0; n < 4; n++)
					{
						int x = xvec[0] + dx4[n];
						int y = yvec[0] + dy4[n];
						if ((x >= 0 && x < width) && (y >= 0 && y < height))
						{
							int nindex = y * width + x;
							if (nlabels[nindex] >= 0) adjlabel = nlabels[nindex];
						}
					}
				}

				int count(1);
				for (int c = 0; c < count; c++)
				{
					for (int n = 0; n < 4; n++)
					{
						int x = xvec[c] + dx4[n];
						int y = yvec[c] + dy4[n];

						if ((x >= 0 && x < width) && (y >= 0 && y < height))
						{
							int nindex = y * width + x;

							if (0 > nlabels[nindex] && labels[oindex] == labels[nindex])
							{
								xvec[count] = x;
								yvec[count] = y;
								nlabels[nindex] = label;
								count++;
							}
						}

					}
				}
				//-------------------------------------------------------
				// If segment size is less then a limit, assign an
				// adjacent label found before, and decrement label count.
				//-------------------------------------------------------
				if (count <= SUPSZ >> 2)
				{
					for (int c = 0; c < count; c++)
					{
						int ind = yvec[c] * width + xvec[c];
						nlabels[ind] = adjlabel;
					}
					label--;
				}
				label++;
			}
			oindex++;
		}
	}
	numlabels = label;

	if (xvec) delete[] xvec;
	if (yvec) delete[] yvec;
}



Image SuperPixels(Image& rgb, int k, double M)
{
	//Converte a imagem para LAB.
	Image Lab = convertImageFromRGB2Lab(rgb);

	//Calcula o numero de pixels cada superpixel.
	double s = sqrt(Lab.getH()*Lab.getW() / k);

	//Inicializa os os clusters. 
	Cluster* clusters;

	int new_k = initializeClusters(clusters, Lab, k, s);

	// Executa o algoritmo.
	int *labels = new int[Lab.getH()*Lab.getW()];

	performSuperPixelsAlgorithm(Lab, clusters, labels, k, M, s);

	// Aloca e inicializa labels.

	int size = Lab.getH()*Lab.getW();
		
	    int* nlabels = new int[size];
		int numLabels = 0;
	    enforceLabelConnectivity( labels, Lab.getW(), Lab.getH(), nlabels, numLabels, k );
	    for (int i = 0; i < size; i++)
	        labels[i] = nlabels[i];

	if (nlabels)
	    delete [] nlabels;

	//Converte a imagem de volta.

	Image newRgb = convertImageFromLAB2RGB(Lab);

	//Desenha os contornos. Deve passar a imagem em rgb e o vetor de labels.
	drawContoursAroundSegments( newRgb, labels );

	delete clusters;

	return newRgb;
}


	
/*
*
*/
int main(void)
{

	Image l;
	if (l.readBMP("estrela.bmp"))
	{
		printf("Leitura executada com sucesso\n");
	}
	 
	Image newL = SuperPixels(l, 512, 20);

	if (newL.writeBMP("result.bmp"))
	{
		printf("Escrita executada com sucesso\n");
	}
	 
	return 0;
}

