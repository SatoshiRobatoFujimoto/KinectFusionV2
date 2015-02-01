// Fusion.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
// This source code is licensed under the MIT license. Please see the License in License.txt.
// "This is preliminary software and/or hardware and APIs are preliminary and subject to change."
//

//#include "stdafx.h"
#include <Windows.h>
#include <Kinect.h>
#include <NuiKinectFusionApi.h>
// Quote from Kinect for Windows SDK v2.0 Developer Preview - Samples/Native/KinectFusionExplorer-D2D, and Partial Modification
// KinectFusionHelper is: Copyright (c) Microsoft Corporation. All rights reserved.
#include "KinectFusionHelper.h"
#include <opencv2/opencv.hpp>
#include <ppl.h> // for Concurrency::parallel_for


template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL){
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

void ResetReconstruction(INuiFusionColorReconstruction* pReconstruction, Matrix4* pMat)
{
	std::cout << "Reset Reconstruction" << std::endl;
	SetIdentityMatrix(*pMat);
	pReconstruction->ResetReconstruction(pMat, nullptr);
}

int main(int argc, char **argv)
{
	cv::setUseOptimized(true);

	// Sensor
	IKinectSensor* pSensor;
	HRESULT hResult = S_OK;
	hResult = GetDefaultKinectSensor(&pSensor);
	if (FAILED(hResult)){
		std::cerr << "Error : GetDefaultKinectSensor" << std::endl;
		return -1;
	}

	hResult = pSensor->Open();
	if (FAILED(hResult)){
		std::cerr << "Error : IKinectSensor::Open()" << std::endl;
		return -1;
	}

	// Source
	IColorFrameSource* pColorSource;
	hResult = pSensor->get_ColorFrameSource(&pColorSource);
	if (FAILED(hResult)){
		std::cerr << "Error : IKinectSensor::get_ColorFrameSource()" << std::endl;
		return -1;
	}

	IDepthFrameSource* pDepthSource;
	hResult = pSensor->get_DepthFrameSource(&pDepthSource);
	if (FAILED(hResult)){
		std::cerr << "Error : IKinectSensor::get_DepthFrameSource()" << std::endl;
		return -1;
	}

	// Reader
	IColorFrameReader* pColorReader;
	hResult = pColorSource->OpenReader(&pColorReader);
	if (FAILED(hResult)){
		std::cerr << "Error : IColorFrameSource::OpenReader()" << std::endl;
		return -1;
	}

	IDepthFrameReader* pDepthReader;
	hResult = pDepthSource->OpenReader(&pDepthReader);
	if (FAILED(hResult)){
		std::cerr << "Error : IDepthFrameSource::OpenReader()" << std::endl;
		return -1;
	}

	// Description
	IFrameDescription* pColorDescription;
	hResult = pColorSource->get_FrameDescription(&pColorDescription);
	if (FAILED(hResult)){
		std::cerr << "Error : IColorFrameSource::get_FrameDescription()" << std::endl;
		return -1;
	}

	int colorWidth = 0;
	int colorHeight = 0;
	pColorDescription->get_Width(&colorWidth); // 1920
	pColorDescription->get_Height(&colorHeight); // 1080
	unsigned int colorBufferSize = colorWidth * colorHeight * 4 * sizeof(unsigned char);
	cv::Mat colorBufferMat(colorHeight, colorWidth, CV_8UC4);

	IFrameDescription* pDepthDescription;
	hResult = pDepthSource->get_FrameDescription(&pDepthDescription);
	if (FAILED(hResult)){
		std::cerr << "Error : IDepthFrameSource::get_FrameDescription()" << std::endl;
		return -1;
	}

	int depthWidth = 0;
	int depthHeight = 0;
	pDepthDescription->get_Width(&depthWidth); // 512
	pDepthDescription->get_Height(&depthHeight); // 424
	unsigned int depthBufferSize = depthWidth * depthHeight * sizeof(unsigned short);

	cv::Mat depthBufferMat(depthHeight, depthWidth, CV_16UC1);
	cv::Mat depthMat(depthHeight, depthWidth, CV_8UC1);
	cv::namedWindow("Depth");

	cv::Mat colorMat(depthHeight, depthWidth, CV_8UC4);
	cv::namedWindow("Color");

	//Coordinate Mapper
	ICoordinateMapper* pCoordinateMapper;
	hResult = pSensor->get_CoordinateMapper(&pCoordinateMapper);
	if (FAILED(hResult)){
		std::cerr << "Error : IKinectSensor::get_CoordinateMapper()" << std::endl;
		return -1;
	}
	cv::namedWindow("CoordinateMapper");

	unsigned short minDepth, maxDepth;
	pDepthSource->get_DepthMinReliableDistance(&minDepth);
	pDepthSource->get_DepthMaxReliableDistance(&maxDepth);

	// Create Reconstruction
	NUI_FUSION_RECONSTRUCTION_PARAMETERS reconstructionParameter;
	reconstructionParameter.voxelsPerMeter = 256;
	reconstructionParameter.voxelCountX = 512;
	reconstructionParameter.voxelCountY = 384;
	reconstructionParameter.voxelCountZ = 512;

	Matrix4 worldToCameraTransform;
	SetIdentityMatrix(worldToCameraTransform);
	INuiFusionColorReconstruction* pReconstruction;
	hResult = NuiFusionCreateColorReconstruction(&reconstructionParameter, NUI_FUSION_RECONSTRUCTION_PROCESSOR_TYPE_AMP, -1, &worldToCameraTransform, &pReconstruction);
	if (FAILED(hResult)){
		std::cerr << "Error : NuiFusionCreateReconstruction()" << std::endl;
		return -1;
	}

	// Create Image Frame
	// Depth
	NUI_FUSION_IMAGE_FRAME* pDepthFloatImageFrame;
	hResult = NuiFusionCreateImageFrame(NUI_FUSION_IMAGE_TYPE_FLOAT, depthWidth, depthHeight, nullptr, &pDepthFloatImageFrame);
	if (FAILED(hResult)){
		std::cerr << "Error : NuiFusionCreateImageFrame( FLOAT )" << std::endl;
		return -1;
	}

	// SmoothDepth
	NUI_FUSION_IMAGE_FRAME* pSmoothDepthFloatImageFrame;
	hResult = NuiFusionCreateImageFrame(NUI_FUSION_IMAGE_TYPE_FLOAT, depthWidth, depthHeight, nullptr, &pSmoothDepthFloatImageFrame);
	if (FAILED(hResult)){
		std::cerr << "Error : NuiFusionCreateImageFrame( FLOAT )" << std::endl;
		return -1;
	}

	// Point Cloud
	NUI_FUSION_IMAGE_FRAME* pPointCloudImageFrame;
	hResult = NuiFusionCreateImageFrame(NUI_FUSION_IMAGE_TYPE_POINT_CLOUD, depthWidth, depthHeight, nullptr, &pPointCloudImageFrame);
	if (FAILED(hResult)){
		std::cerr << "Error : NuiFusionCreateImageFrame( POINT_CLOUD )" << std::endl;
		return -1;
	}

	// Color
	NUI_FUSION_IMAGE_FRAME* pColorImageFrame;
	hResult = NuiFusionCreateImageFrame(NUI_FUSION_IMAGE_TYPE_COLOR, depthWidth, depthHeight, nullptr, &pColorImageFrame);
	if (FAILED(hResult)){
		std::cerr << "Error : NuiFusionCreateImageFrame( COLOR )" << std::endl;
		return -1;
	}

	// Surface
	//NUI_FUSION_IMAGE_FRAME* pSurfaceImageFrame;
	//hResult = NuiFusionCreateImageFrame(NUI_FUSION_IMAGE_TYPE_COLOR, depthWidth, depthHeight, nullptr, &pSurfaceImageFrame);
	//if (FAILED(hResult)){
	//	std::cerr << "Error : NuiFusionCreateImageFrame( COLOR )" << std::endl;
	//	return -1;
	//}

	//// Normal
	//NUI_FUSION_IMAGE_FRAME* pNormalImageFrame;
	//hResult = NuiFusionCreateImageFrame(NUI_FUSION_IMAGE_TYPE_COLOR, depthWidth, depthHeight, nullptr, &pNormalImageFrame);
	//if (FAILED(hResult)){
	//	std::cerr << "Error : NuiFusionCreateImageFrame( COLOR )" << std::endl;
	//	return -1;
	//}

	cv::namedWindow("Surface");
	//cv::namedWindow("Normal");

	std::vector<Matrix4> worldToCameraTransformSeq;

	while (1){
		// Frame
		// Color Frame
		IColorFrame* pColorFrame = nullptr;
		hResult = pColorReader->AcquireLatestFrame(&pColorFrame);
		if (SUCCEEDED(hResult)){
			if (SUCCEEDED(hResult)){
				hResult = pColorFrame->CopyConvertedFrameDataToArray(colorBufferSize, reinterpret_cast<BYTE*>(colorBufferMat.data), ColorImageFormat::ColorImageFormat_Bgra);
				if (SUCCEEDED(hResult)){
					cv::resize(colorBufferMat, colorMat, cv::Size(depthWidth, depthHeight));
				}
			}
		}

		// Depth Frame
		IDepthFrameArrivedEventArgs* pDepthArgs = nullptr;
		IDepthFrameReference* pDepthReference = nullptr;
		IDepthFrame* pDepthFrame = nullptr;
		hResult = pDepthReader->AcquireLatestFrame(&pDepthFrame);
		if (SUCCEEDED(hResult)){
			if (SUCCEEDED(hResult)){
				hResult = pDepthFrame->AccessUnderlyingBuffer(&depthBufferSize, reinterpret_cast<UINT16**>(&depthBufferMat.data));
				if (SUCCEEDED(hResult)){
					depthBufferMat.convertTo(depthMat, CV_8U, -255.0f / 8000.0f, 255.0f);
					hResult = pReconstruction->DepthToDepthFloatFrame(reinterpret_cast<UINT16*>(depthBufferMat.data), depthWidth * depthHeight * sizeof(UINT16), pDepthFloatImageFrame, NUI_FUSION_DEFAULT_MINIMUM_DEPTH/* 0.5[m] */, NUI_FUSION_DEFAULT_MAXIMUM_DEPTH/* 8.0[m] */, true);
					if (FAILED(hResult)){
						std::cerr << "Error :INuiFusionReconstruction::DepthToDepthFloatFrame()" << std::endl;
						return -1;
					}
				}
			}
		}

		// ColorデータからNUI_FUSION_IMAGE_FRAMEに変換
		int* pSrc = reinterpret_cast<int*>(colorBufferMat.data);
		int* pDst = reinterpret_cast<int*>(pColorImageFrame->pFrameBuffer->pBits);
		cv::Mat coordinateMapperMat = cv::Mat::zeros(depthHeight, depthWidth, CV_8UC4);

		if (SUCCEEDED(hResult)){
			std::vector<ColorSpacePoint> colorSpacePoints(depthWidth * depthHeight);
			hResult = pCoordinateMapper->MapDepthFrameToColorSpace(depthWidth * depthHeight, reinterpret_cast<UINT16*>(depthBufferMat.data), depthWidth * depthHeight, &colorSpacePoints[0]);
			if (SUCCEEDED(hResult)){
				Concurrency::parallel_for(0, depthHeight, [&](int y){
					for (int x = 0; x < depthWidth; x++){
						unsigned int index = y * depthWidth + x;
						ColorSpacePoint point = colorSpacePoints[index];
						int colorX = static_cast<int>(std::floor(point.X + 0.5));
						int colorY = static_cast<int>(std::floor(point.Y + 0.5));
						unsigned short depth = depthBufferMat.at<unsigned short>(y, x);
						if ((colorX >= 0) && (colorX < colorWidth) && (colorY >= 0) && (colorY < colorHeight) && (depth >= minDepth) && (depth <= 4500/*maxDepth*/)){
							coordinateMapperMat.at<cv::Vec4b>(y, x) = colorBufferMat.at<cv::Vec4b>(colorY, colorX);
							pDst[index] = pSrc[colorY * colorWidth + colorX];
						}
						else{
							pDst[index] = 0;
						}
					}
				});
			}
		}

		// Smooting Depth Image Frame
		hResult = pReconstruction->SmoothDepthFloatFrame(pDepthFloatImageFrame, pSmoothDepthFloatImageFrame, 1, 0.04f);
		if (FAILED(hResult)){
			std::cerr << "Error :INuiFusionReconstruction::SmoothDepthFloatFrame" << std::endl;
			return -1;
		}

		// Reconstruction Process
		pReconstruction->GetCurrentWorldToCameraTransform(&worldToCameraTransform);
		hResult = pReconstruction->ProcessFrame(pSmoothDepthFloatImageFrame, pColorImageFrame, NUI_FUSION_DEFAULT_ALIGN_ITERATION_COUNT, NUI_FUSION_DEFAULT_INTEGRATION_WEIGHT, NUI_FUSION_DEFAULT_COLOR_INTEGRATION_OF_ALL_ANGLES, nullptr, &worldToCameraTransform);
		
		//std::cout << "R = " << worldToCameraTransform.M11 << " " << worldToCameraTransform.M12 << " " << worldToCameraTransform.M13 << " " << worldToCameraTransform.M21 << " " << worldToCameraTransform.M22 << " " << worldToCameraTransform.M23 << " " << worldToCameraTransform.M31 << " " << worldToCameraTransform.M32 << " " << worldToCameraTransform.M33 << std::endl;

		//std::cout << "T = " << worldToCameraTransform.M41 << " " << worldToCameraTransform.M42 << " " << worldToCameraTransform.M43 << std::endl;

		worldToCameraTransformSeq.push_back(worldToCameraTransform);


		if (FAILED(hResult)){
			static int errorCount = 0;
			errorCount++;
			if (errorCount >= 100) {
				errorCount = 0;
				ResetReconstruction(pReconstruction, &worldToCameraTransform);
				worldToCameraTransformSeq.resize(0);
			}
		}

		// Calculate Point Cloud
		hResult = pReconstruction->CalculatePointCloud(pPointCloudImageFrame, pColorImageFrame, &worldToCameraTransform);
		if (FAILED(hResult)){
			std::cerr << "Error : CalculatePointCloud" << std::endl;
			return -1;
		}

		//// Shading Point Clouid
		//Matrix4 worldToBGRTransform = { 0.0f };
		//worldToBGRTransform.M11 = reconstructionParameter.voxelsPerMeter / reconstructionParameter.voxelCountX;
		//worldToBGRTransform.M22 = reconstructionParameter.voxelsPerMeter / reconstructionParameter.voxelCountY;
		//worldToBGRTransform.M33 = reconstructionParameter.voxelsPerMeter / reconstructionParameter.voxelCountZ;
		//worldToBGRTransform.M41 = 0.5f;
		//worldToBGRTransform.M42 = 0.5f;
		//worldToBGRTransform.M43 = 0.0f;
		//worldToBGRTransform.M44 = 1.0f;

		//hResult = NuiFusionShadePointCloud(pPointCloudImageFrame, &worldToCameraTransform, &worldToBGRTransform, pSurfaceImageFrame, pNormalImageFrame);
		//if (FAILED(hResult)){
		//	std::cerr << "Error : NuiFusionShadePointCloud" << std::endl;
		//	return -1;
		//}

		cv::Mat surfaceMat(depthHeight, depthWidth, CV_8UC4, pColorImageFrame->pFrameBuffer->pBits);
		//cv::Mat normalMat(depthHeight, depthWidth, CV_8UC4, pNormalImageFrame->pFrameBuffer->pBits);

		cv::imshow("Color", colorMat);
		cv::imshow("Depth", depthMat);
		cv::imshow("CoordinateMapper", coordinateMapperMat);
		cv::imshow("Surface", surfaceMat);
		//cv::imshow("Normal", normalMat);

		surfaceMat.release();
		coordinateMapperMat.release();
		int key = cv::waitKey(10);
		if (key == VK_ESCAPE){
			break;
		}
		else if (key == 'r'){
			ResetReconstruction(pReconstruction, &worldToCameraTransform);
			worldToCameraTransformSeq.resize(0);
		}
		else if (key == 's'){
			INuiFusionColorMesh *pMesh = nullptr;
			hResult = pReconstruction->CalculateMesh(1, &pMesh);
			if (SUCCEEDED(hResult)){
				std::cout << "Save Mesh File" << std::endl;
				char* fileName = "mesh.ply";
				WriteAsciiPlyMeshFile(pMesh, fileName, true, true);
				//fileName = "mesh.stl";
				//WriteBinarySTLMeshFile(pMesh, fileName, true);
				//fileName = "mesh.obj";
				//WriteAsciiObjMeshFile(pMesh, fileName, true);
				pMesh->Release();

				std::cout << "Save Camera Pose" << std::endl;
				fileName = "CameraPose.ply";
				WriteAsciiPlyCameraPoseFile(&worldToCameraTransformSeq, fileName, true, true);
			}
		}
		SafeRelease(pColorFrame);
		SafeRelease(pDepthFrame);

	}

	SafeRelease(pColorSource);
	SafeRelease(pDepthSource);
	SafeRelease(pColorReader);
	SafeRelease(pDepthReader);
	SafeRelease(pColorDescription);
	SafeRelease(pDepthDescription);
	SafeRelease(pCoordinateMapper);
	SafeRelease(pReconstruction);
	NuiFusionReleaseImageFrame(pDepthFloatImageFrame);
	NuiFusionReleaseImageFrame(pSmoothDepthFloatImageFrame);
	NuiFusionReleaseImageFrame(pColorImageFrame);
	NuiFusionReleaseImageFrame(pPointCloudImageFrame);
	//NuiFusionReleaseImageFrame(pSurfaceImageFrame);
	//NuiFusionReleaseImageFrame(pNormalImageFrame);
	if (pSensor){
		pSensor->Close();
	}
	SafeRelease(pSensor);
	cv::destroyAllWindows();

	return 0;
}

