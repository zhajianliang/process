#include "videocommon.h"
#include "osp.h"
#include "logcomponent.h"
//#include "dshowdraw.h"
extern "C" { 
#include "libyuv.h"
#if _MSC_VER >= 1900 //vs2015
#include "jpeglib.h"
#endif
}
/*=========================================================      
		<< using声明 与 using指示 的区别>>

*using指示注入来自一个命名空间的所有名字，它的使用是靠不住的：
	只用一个语句，命名空间的所有成员名就突然可见了。如果应用程序使用许多库，
	并且用using指示使得这些库中的名字可见，那么，全局命名空间污染问题就重新出现。

*引入库的新版本的时候，正在工作的程序可能会编译失败。
	如果新版本的引入一个与应用程序正在使用名字冲突的名字，就会引发命名空间污染。

*由using指示引起的二义性错误只能在使用处检测。

*相对于依赖于using指示，对程序中使用的每个命名空间名字使用using声明更好，
	这样做减少注入到程序中的名字数目，由using声明引起的二义性错误在声明点而不是使用点检测，
	因此更容易发现和修正。
--------------------- 
作者：ljianhui 
来源：CSDN 
原文：https://blog.csdn.net/ljianhui/article/details/7752580 
版权声明：本文为博主原创文章，转载请附上博文链接！
========================================================================*/

#ifdef __cplusplus
//using namespace libyuv; //注掉此处是为了 避免使用using指示
using libyuv::kFilterNone;
#if _MSC_VER <= 1600 //vs2010
using libyuv::RotationMode;
#elif _MSC_VER >= 1900 //vs2015
//using libyuv::RotationModeEnum;
#endif
using libyuv::I420Scale;
using libyuv::I420Rotate;
using libyuv::I420ToRGB24;
using libyuv::I420ToABGR;
using libyuv::YUY2ToI420;
using libyuv::RGB24ToI420;
using libyuv::UYVYToI420;
using libyuv::I420ToNV12;
#ifdef LIBYUV_JPEG
using libyuv::MJPGToI420;
#endif

#endif
//============= 封装libyuv中接口，方便使用 =====================
s32 libyuv_I420Scale(TVidRawData* ptSrcI420, TVidRawData* ptDstI420, TPicZoomStyle eType)
{
	if (ptSrcI420 == NULL || ptDstI420 == NULL)
	{
		MError("Input param is invalid. ptSrcI420=0x%p, ptDstI420=0x%p\n", ptSrcI420, ptDstI420);
		return -1;
	}
	if (ptSrcI420->dwWidth == 0 || ptSrcI420->dwHeight == 0 || ptDstI420->dwWidth == 0 || ptDstI420->dwHeight == 0)
	{
		MError("Input param is invalid. SrcSize=%dx%d, DstSize=%dx%d\n", ptSrcI420->dwWidth, ptSrcI420->dwHeight, ptDstI420->dwWidth, ptDstI420->dwHeight);
		return -2;
	}

	u32 dwZoomWidth = ptDstI420->dwWidth; //等比缩放后的宽
	u32 dwZoomHeight = ptDstI420->dwHeight; //等比缩放后的高
	float fZoomScaleWidth = ((float)ptDstI420->dwWidth) / ptSrcI420->dwWidth;
	float fZoomScaleHeight = ((float)ptDstI420->dwHeight) / ptSrcI420->dwHeight;
	u32 dwDiffWidth = 0; //黑边宽，用于 缩放——左右加黑边
	u32 dwDiffHeight = 0; //黑边高，用于 缩放——上下加黑边

	u8 byFlag = 0; //默认缩放方式——不加黑边
	if (fZoomScaleWidth > fZoomScaleHeight)
	{
		byFlag = 1; // 缩放——左右加黑边
		dwZoomWidth = fZoomScaleHeight * ptSrcI420->dwWidth;
		dwZoomHeight = ptDstI420->dwHeight;
		dwDiffWidth = (ptDstI420->dwWidth - dwZoomWidth) / 2;
	}
	else if (fZoomScaleWidth < fZoomScaleHeight)
	{
		byFlag = 2; // 缩放——上下加黑边
		dwZoomWidth = ptDstI420->dwWidth;
		dwZoomHeight = fZoomScaleWidth * ptSrcI420->dwHeight;
		dwDiffHeight = (ptDstI420->dwHeight - dwZoomHeight) / 2;
	}
	//如果不是等比拉伸加黑边模式，那么统统设置成不等比拉伸
	if (eType != PIC_ZOOM_TWO)
	{
		byFlag = 0;
		dwZoomWidth = ptDstI420->dwWidth;
		dwZoomHeight = ptDstI420->dwHeight;
	}
	//确保加的黑边是黑色的
	if (byFlag != 0)
	{
		memset(ptDstI420->pDataY, 0, ptDstI420->dwWidth*ptDstI420->dwHeight);
		memset(ptDstI420->pDataU, 0x80, ptDstI420->dwWidth*ptDstI420->dwHeight / 4);
		memset(ptDstI420->pDataV, 0x80, ptDstI420->dwWidth*ptDstI420->dwHeight / 4);
	}


	s32 nRet = 0;
	if (byFlag == 0) //默认缩放方式——不加黑边
	{
		nRet = I420Scale(
			ptSrcI420->pDataY, ptSrcI420->dwPitchY,
			ptSrcI420->pDataU, ptSrcI420->dwPitchU,
			ptSrcI420->pDataV, ptSrcI420->dwPitchV,
			ptSrcI420->dwWidth, ptSrcI420->dwHeight,

			ptDstI420->pDataY, ptDstI420->dwPitchY,
			ptDstI420->pDataU, ptDstI420->dwPitchU,
			ptDstI420->pDataV, ptDstI420->dwPitchV,
			dwZoomWidth, dwZoomHeight,
			kFilterNone);
	}
	else if (byFlag == 1) // 缩放——左右加黑边
	{
		nRet = I420Scale(
			ptSrcI420->pDataY, ptSrcI420->dwPitchY,
			ptSrcI420->pDataU, ptSrcI420->dwPitchU,
			ptSrcI420->pDataV, ptSrcI420->dwPitchV,
			ptSrcI420->dwWidth, ptSrcI420->dwHeight,

			ptDstI420->pDataY + dwDiffWidth, ptDstI420->dwPitchY,
			ptDstI420->pDataU + dwDiffWidth / 2, ptDstI420->dwPitchU,
			ptDstI420->pDataV + dwDiffWidth / 2, ptDstI420->dwPitchV,
			dwZoomWidth, dwZoomHeight,
			kFilterNone);
	}
	else if (byFlag == 2) // 缩放——上下加黑边
	{
		nRet = I420Scale(
			ptSrcI420->pDataY, ptSrcI420->dwPitchY,
			ptSrcI420->pDataU, ptSrcI420->dwPitchU,
			ptSrcI420->pDataV, ptSrcI420->dwPitchV,
			ptSrcI420->dwWidth, ptSrcI420->dwHeight,

			ptDstI420->pDataY + dwZoomWidth * dwDiffHeight, ptDstI420->dwPitchY,
			ptDstI420->pDataU + dwZoomWidth * dwDiffHeight / 4, ptDstI420->dwPitchU,
			ptDstI420->pDataV + dwZoomWidth * dwDiffHeight / 4, ptDstI420->dwPitchV,
			dwZoomWidth, dwZoomHeight,
			kFilterNone);
	}
	
	return nRet;
}

//s32 libyuv_I420Rotate(TVidRawData* ptSrcI420, TVidRawData* ptDstI420, u32 dwRotateDegrees)
//{
//	if (ptSrcI420 == NULL || ptDstI420 == NULL)
//	{
//		MError("Input param is invalid. ptSrcI420=0x%p, ptDstI420=0x%p\n", ptSrcI420, ptDstI420);
//		return -1;
//	}
//#if _MSC_VER <= 1600 //vs2010
//	RotationMode enRotationMode = (RotationMode)dwRotateDegrees;
//#elif _MSC_VER >= 1900 //vs2015
//	RotationModeEnum enRotationMode = (RotationModeEnum)dwRotateDegrees;
//#endif
//	s32 nRet = I420Rotate(
//		ptSrcI420->pDataY, ptSrcI420->dwPitchY,
//		ptSrcI420->pDataU, ptSrcI420->dwPitchU,
//		ptSrcI420->pDataV, ptSrcI420->dwPitchV,
//
//		ptDstI420->pDataY, ptDstI420->dwPitchY,
//		ptDstI420->pDataU, ptDstI420->dwPitchU,
//		ptDstI420->pDataV, ptDstI420->dwPitchV,
//		ptDstI420->dwWidth, ptDstI420->dwHeight,
//		enRotationMode);
//
//	return nRet;
//}

s32 libyuv_I420ToRGB24(TVidRawData* ptSrcI420, u8* pDstRGB24, u32 dwPitchRGB24)
{
	if (ptSrcI420 == NULL || pDstRGB24 == NULL)
	{
		MError("Input param is invalid. ptSrcI420=0x%p, pDstRGB24=0x%p\n", ptSrcI420, pDstRGB24);
		return -1;
	}
	s32 nRet = I420ToRGB24(
		ptSrcI420->pDataY, ptSrcI420->dwPitchY,
		ptSrcI420->pDataU, ptSrcI420->dwPitchU,
		ptSrcI420->pDataV, ptSrcI420->dwPitchV,
		pDstRGB24, dwPitchRGB24,
		ptSrcI420->dwWidth, ptSrcI420->dwHeight);

	return nRet;
}
//I420ToNV12
s32 libyuv_I420ToNV12(TVidRawData* ptSrcI420, TVidRawData* ptDstNV12)
{
	if (ptSrcI420 == NULL || ptDstNV12 == NULL)
	{
		MError("Input param is invalid. ptSrcI420=0x%p, ptDstNV12=0x%p\n", ptSrcI420, ptDstNV12);
		return -1;
	}
	if (ptSrcI420->dwWidth != ptDstNV12->dwWidth || ptSrcI420->dwHeight != ptDstNV12->dwHeight)
	{
		MError("Input param is invalid. Src wxh != Dst wxh\n", ptSrcI420->dwWidth, ptSrcI420->dwHeight, ptDstNV12->dwWidth, ptDstNV12->dwHeight);
		return -1;
	}
	s32 nRet = I420ToNV12(
		ptSrcI420->pDataY, ptSrcI420->dwPitchY,
		ptSrcI420->pDataU, ptSrcI420->dwPitchU,
		ptSrcI420->pDataV, ptSrcI420->dwPitchV,
		ptDstNV12->pDataY, ptDstNV12->dwWidth,
		ptDstNV12->pDataU, ptDstNV12->dwWidth,
		ptSrcI420->dwWidth, ptSrcI420->dwHeight);

	return nRet;
}

s32 libyuv_YUY2ToI420(u8* pSrcYUY2, u32 dwPitchYUY2, TVidRawData* ptDstI420)
{
	if (pSrcYUY2 == NULL || ptDstI420 == NULL)
	{
		MError("Input param is invalid. pSrcYUY2=0x%p, ptDstI420=0x%p\n", pSrcYUY2, ptDstI420);
		return -1;
	}
	s32 nRet = YUY2ToI420(
		pSrcYUY2, dwPitchYUY2,
		ptDstI420->pDataY, ptDstI420->dwPitchY,
		ptDstI420->pDataU, ptDstI420->dwPitchU,
		ptDstI420->pDataV, ptDstI420->dwPitchV,
		ptDstI420->dwWidth, ptDstI420->dwHeight);

	return nRet;
}

s32 libyuv_RGB24ToI420(u8* pSrcRGB24, u32 dwPitchRGB24, TVidRawData* ptDstI420)
{
	if (pSrcRGB24 == NULL || ptDstI420 == NULL)
	{
		MError("Input param is invalid. pSrcRGB24=0x%p, ptDstI420=0x%p\n", pSrcRGB24, ptDstI420);
		return -1;
	}
	s32 nRet = RGB24ToI420(
		pSrcRGB24, dwPitchRGB24,
		ptDstI420->pDataY, ptDstI420->dwPitchY,
		ptDstI420->pDataU, ptDstI420->dwPitchU,
		ptDstI420->pDataV, ptDstI420->dwPitchV,
		ptDstI420->dwWidth, ptDstI420->dwHeight);

	return nRet;
}

s32 libyuv_UYVYToI420(u8* pSrcUYVY, u32 dwPitchUYVY, TVidRawData* ptDstI420)
{
	if (pSrcUYVY == NULL || ptDstI420 == NULL)
	{
		MError("Input param is invalid. pSrcUYVY=0x%p, ptDstI420=0x%p\n", pSrcUYVY, ptDstI420);
		return -1;
	}
	s32 nRet = UYVYToI420(
		pSrcUYVY, dwPitchUYVY,
		ptDstI420->pDataY, ptDstI420->dwPitchY,
		ptDstI420->pDataU, ptDstI420->dwPitchU,
		ptDstI420->pDataV, ptDstI420->dwPitchV,
		ptDstI420->dwWidth, ptDstI420->dwHeight);

	return nRet;
}

s32 libyuv_MJPGToI420(u8* pSrcMJPG, u32 dwSizeMJPG, TVidRawData* ptDstI420)
{
#ifdef LIBYUV_JPEG
	if (pSrcMJPG == NULL || ptDstI420 == NULL)
	{
		MError("Input param is invalid. pSrcYUY2=0x%p, ptDstI420=0x%p\n", pSrcMJPG, ptDstI420);
		return -1;
	}
	s32 nRet = MJPGToI420(
		pSrcMJPG, dwSizeMJPG,
		ptDstI420->pDataY, ptDstI420->dwPitchY,
		ptDstI420->pDataU, ptDstI420->dwPitchU,
		ptDstI420->pDataV, ptDstI420->dwPitchV,
		ptDstI420->dwWidth, ptDstI420->dwHeight,
		ptDstI420->dwWidth, ptDstI420->dwHeight);

	return nRet;
#else
	return -1;
#endif
}

s32 libyuv_I420ToABGR(u8* pSrcYVU, u8* pDst, u32 dwWidth, u32 dwHeight)
{
	if ((!pSrcYVU) || (!pDst))
	{
		return -1;
	}
	s32 nRet = I420ToABGR(pSrcYVU, dwWidth,  pSrcYVU + dwWidth * dwHeight, dwWidth / 2, pSrcYVU + dwWidth * dwHeight + (dwWidth / 2) * (dwHeight / 2), dwWidth / 2,
		pDst, dwWidth * 2 *  2, dwWidth, dwHeight);
	return nRet;
}
//============= 封装libjpeg中接口，方便使用 =====================
//RGB2JPG
s32 libjpeg_RGBToJpegFile(char* fileName, BYTE *dataBuf, UINT widthPix, UINT height, BOOL color, int quality)
{
	return 0;
}

s32 FillI420Struct(TVidRawData* ptI420, u8* pDataY, u32 dwWidth, u32 dwHeight, u32 dwFrameRate, u32 dwTimeStamp)
{
	if (ptI420 == NULL || pDataY == NULL || dwWidth == 0 || dwHeight == 0)
	{
		MError("Input param is invalid. ptI420=0x%p, pDataY=0x%p, %dx%d\n", ptI420, pDataY, dwWidth, dwHeight);
		return -1;
	}

	ptI420->dwCompression = (u32)en_VideoI420;
	ptI420->dwWidth = dwWidth;
	ptI420->dwHeight = dwHeight;
	ptI420->dwPitchY = ptI420->dwWidth;
	ptI420->dwPitchU = ptI420->dwWidth >> 1;
	ptI420->dwPitchV = ptI420->dwWidth >> 1;
	ptI420->pDataY = pDataY;
	ptI420->pDataU = ptI420->pDataY + ptI420->dwWidth*ptI420->dwHeight;
	ptI420->pDataV = ptI420->pDataU + ptI420->dwWidth*ptI420->dwHeight / 4;
	if (ptI420->dwDataSize == 0)
	{
		ptI420->dwDataSize = ptI420->dwWidth*ptI420->dwHeight * 3 / 2;
	}
	if (dwFrameRate)
	{
		ptI420->dwFrameRate = dwFrameRate;
	}
	if (dwTimeStamp)
	{
		ptI420->dwTimeStamp = dwTimeStamp;
	}

	return 0;
}
//#if _MSC_VER >= 1900 //vs2015
//int rgb2jpg(char *jpg_file, char *pdata, int width, int height)
//{
//	int depth = 3;
//	JSAMPROW row_pointer[1];
//	struct jpeg_compress_struct cinfo;
//	struct jpeg_error_mgr jerr;
//	FILE *outfile;
//
//	if ((outfile = fopen(jpg_file, "wb")) == NULL)
//	{
//		return -1;
//	}
//
//	cinfo.err = jpeg_std_error(&jerr);
//	jpeg_create_compress(&cinfo);
//	jpeg_stdio_dest(&cinfo, outfile);
//
//	cinfo.image_width = width;
//	cinfo.image_height = height;
//	cinfo.input_components = depth;
//	cinfo.in_color_space = JCS_RGB;
//	jpeg_set_defaults(&cinfo);
//
//	jpeg_set_quality(&cinfo, 50/*JPEG_QUALITY*/, TRUE);
//	jpeg_start_compress(&cinfo, TRUE);
//
//	int row_stride = width * depth;
//	while (cinfo.next_scanline < cinfo.image_height)
//	{
//		row_pointer[0] = (JSAMPROW)(pdata + cinfo.next_scanline * row_stride);
//		jpeg_write_scanlines(&cinfo, row_pointer, 1);
//	}
//
//	jpeg_finish_compress(&cinfo);
//	jpeg_destroy_compress(&cinfo);
//	fclose(outfile);
//
//	return 0;
//}
//#endif
u32 BmpHeight(u8 * pBmp)
{
	if(pBmp == NULL)
	{
		return 0;
	}
	//return *(s32*)(pBmp+22);
	return ((*((u8 *)pBmp+0x16)) + ((*((u8 *)pBmp+0x17))<<8) + ((*((u8 *)pBmp+0x18))<<16) + ((*((u8 *)pBmp+0x19))<<24));
}
u32 BmpWidth(u8 * pBmp)
{
	if(pBmp == NULL)
	{
		return 0;
	}
	//return *(s32*)(pBmp+18);
	return ((*((u8 *)pBmp+0x12)) + ((*((u8 *)pBmp+0x13))<<8) + ((*((u8 *)pBmp+0x14))<<16) + ((*((u8 *)pBmp+0x15))<<24));
}

u32 BmpBitCount(u8 * pBmp)
{
	if (pBmp == NULL)
	{
		return 0;
	}
	//return *(u16*)(pBmp+28);
	return ((*((u8 *)pBmp+0x1c))+ ((*((u8 *)pBmp+0x1d))<<8));
}

u32 BmpFileSize(u8* pBmp)
{
	if (pBmp == NULL)
	{
		return 0;
	}
	return *(u32*)(pBmp+2);
}

u32 BmpDataSize(u8* pBmp)
{
	if (pBmp == NULL)
	{
		return 0;
	}
	return *(u32*)(pBmp+34);
}

u16 ReInitRawYuvData( TVidRawData* pYuvData, u32 dwNewSize)
{
	if (pYuvData == NULL || dwNewSize == 0)
	{
		return -1;
	}
	
	if (pYuvData->pDataY != NULL && pYuvData->dwDataSize != 0
		&& pYuvData->dwDataSize < dwNewSize)
	{
		ReleaseYuvRaw(pYuvData);
	}

	if (pYuvData->dwDataSize == 0 || pYuvData->pDataY == NULL)
	{
		//MPEG-4编码器的输入地址要求16字节对齐
		pYuvData->pBufRealAddress = new u8[dwNewSize + 0x0F];
		CHECK_NEW_PTR(pYuvData->pBufRealAddress)
		pYuvData->pDataY = (u8*)(((u32)(pYuvData->pBufRealAddress) + 0x0F) >> 4 << 4);
		pYuvData->dwDataSize = dwNewSize;
	}

	return 0;
}

u16 ReleaseYuvRaw( TVidRawData* pYuvData )
{
	if (NULL == pYuvData)
	{
		return 0;
	}
	SAFE_DELETE_ARRAY(pYuvData->pBufRealAddress);//此处原来没有释放 pYuvData->pDataY 可能有问题，现添加安全释放  ---李健2018/6/15
	memset(pYuvData, 0, sizeof(TVidRawData));
	return 0;
}
s32 yuyv2yuv(u8 *yuyv, u32 yuyvSize, u8 *yuv, u32 width, u32 height)
{
	//static FILE* pf = fopen("c:\\yuv\\yuy2.yuv", "wb");
	//static u32 dwindex = 0;
	//if (pf)
	//{
	//	dwindex++;
	//	MPrintf("------->yuy2[%d]: %dx%d \n", dwindex, width, height);
	//	u32 realsize = fwrite(yuyv, 1, width*height*2, pf);
	//	if (realsize != width*height*2)
	//	{
	//		MError("=========>>fwrite[%d]: [realsize:%d] < [width*height*2:%d]\n", dwindex, realsize, width*height*2);
	//	}
	//}
	//if (dwindex >= 100)
	//{
	//	SAFE_FCLOSE(pf);
	//}
	//检测输入buffer大小是否正常，避免之后类型转换发生越界崩溃 ---2019/6/14
	if (yuyvSize < width*height*2)
	{
		//MError("=========>> [yuyvSize:%d] < [width*height*2:%d]\n", yuyvSize, width*height*2);
		return -1;
	}
	//else
	//{
	//	MPrintf("=========>> [yuyvSize:%d] >= [width*height*2:%d]\n", yuyvSize, width*height*2);
	//}
	u32 i, j;
	u8 * p = yuyv;
	u8 * u = yuv + width*height;
	u8 * v = yuv + width*height*5/4;

	for (i = 0; i < width * height; i++)
	{
		yuv[i] = yuyv[i*2];
	}

	for (i = 0; i < height; i += 2)//从偶数行取 UV值
	{
		for (j = 0; j < width * 2; j += 4)
		{
			*u++ = p[j+1];
			*v++ = p[j+3];
		}
		p += width*4;
	}
	return 0;
}

u16 YuvFromBmp(wchar_t*  pchFilePath, TVidRawData *pVidData)
{
	if (!pchFilePath)
	{
		return -1;
	}
	FILE * pFile = _wfopen(pchFilePath, L"rb");
	if (!pFile)
	{
		return -1;
	}
	fseek(pFile, 0, SEEK_END);
	u32 dwLen = ftell(pFile);
	if (dwLen <= 54)
	{
		SAFE_FCLOSE(pFile);
		return -1;
	}
	fseek(pFile, 0, SEEK_SET);
	u8* pData = new u8[dwLen];
	if (!pData)
	{
		SAFE_FCLOSE(pFile);
		return -1;
	}
	fread(pData, 1, dwLen, pFile);
	if (*((u16*)pData) != 0x4d42)
	{
		SAFE_DELETE_ARRAY(pData);
		SAFE_FCLOSE(pFile);
		return -1;
	}
	if (24 != BmpBitCount(pData))
	{
		SAFE_DELETE_ARRAY(pData);
		SAFE_FCLOSE(pFile);
		return -1;
	}
	u32 dwWidth  = BmpWidth(pData);
	u32 dwHeight = BmpHeight(pData);
	if (!pVidData->pBufRealAddress)
	{
		pVidData->pBufRealAddress = new u8[dwWidth * dwHeight * 2];
		pVidData->dwDataSize = dwWidth * dwHeight * 2;
		//MPrintf("dwWidth:%d, dwHeight:%d\n", dwWidth, dwHeight);
	}
	else
	{
		if (pVidData->dwDataSize != dwWidth * dwHeight * 2)
		{
			SAFE_DELETE_ARRAY(pVidData->pBufRealAddress);
			pVidData->pBufRealAddress = new u8[dwWidth * dwHeight * 2];
			pVidData->dwDataSize = dwWidth * dwHeight * 2;
		}
	}
	pVidData->pDataY = pVidData->pBufRealAddress;
	pVidData->pDataU = pVidData->pBufRealAddress + dwWidth * dwHeight;
	pVidData->pDataV = pVidData->pBufRealAddress + dwWidth * dwHeight * 5 / 4;
	pVidData->dwPitchY = dwWidth;
	pVidData->dwPitchU = dwWidth / 2;
	pVidData->dwPitchV = dwWidth / 2;
	pVidData->dwCompression = (u32)en_VideoI420;
	pVidData->dwWidth = dwWidth;
	pVidData->dwHeight = dwHeight;
	libyuv_RGB24ToI420(pData + 54, 3 * dwWidth,  pVidData);
	u32 dwNum = 0;
	u8 *pDat = new u8[dwWidth * dwHeight];
	u8 *pDataY = pVidData->pDataY;
	u8 *pDataU = pVidData->pDataU;
	u8* pDataV = pVidData->pDataV;
	for (dwNum = 0; dwNum < dwHeight / 2; dwNum++)
	{
		memcpy(pDat + dwNum * dwWidth, pDataY + (dwHeight - 1 - dwNum) * dwWidth, dwWidth);
		memcpy(pDataY + (dwHeight - 1 - dwNum) * dwWidth, pDataY + dwNum * dwWidth, dwWidth);
		memcpy(pDataY + dwNum * dwWidth, pDat + dwNum * dwWidth, dwWidth);
	}
	for (dwNum = 0; dwNum < dwHeight / 4; dwNum++)
	{
		memcpy(pDat + dwNum * dwWidth / 2, pDataU + (dwHeight / 2 - 1 - dwNum) * dwWidth / 2, dwWidth / 2);
		memcpy(pDataU + (dwHeight / 2 - 1 - dwNum) * dwWidth / 2, pDataU + dwNum * dwWidth / 2, dwWidth / 2);
		memcpy(pDataU + dwNum * dwWidth / 2, pDat + dwNum * dwWidth / 2, dwWidth / 2);
	}
	for (dwNum = 0; dwNum < dwHeight / 4; dwNum++)
	{
		memcpy(pDat + dwNum * dwWidth / 2, pDataV + (dwHeight / 2 - 1 - dwNum) * dwWidth / 2, dwWidth / 2);
		memcpy(pDataV + (dwHeight / 2 - 1 - dwNum) * dwWidth / 2, pDataV + dwNum * dwWidth / 2, dwWidth / 2);
		memcpy(pDataV + dwNum * dwWidth / 2, pDat + dwNum * dwWidth / 2, dwWidth / 2);
	}
	SAFE_DELETE(pDat);
	SAFE_DELETE_ARRAY(pData);
	SAFE_FCLOSE(pFile);
	return 0;
}

u16 ReadYuvDataFromBmp( TVidRawData & tOutPutYuv, wchar_t* pchFilePath, BOOL32& bReloadPic )
{
	//1. 判断是否重新载入
	if (FALSE == bReloadPic)
	{
		return 0;
	}
	if (pchFilePath == NULL)
	{
		MError("Input param is invalid. pchFilePath=NULL\n");
		return -1;
	}
	//2. 先释放之前载入的BMP数据
	ReleaseYuvRaw(&tOutPutYuv);
	//3. 读文件重新载入BMP数据
	FILE* pBmpFile = _wfopen(pchFilePath,L"rb");
	if (NULL == pBmpFile)
	{
		MError("fopen(\"%s\") failed.\n", pchFilePath);
		return -2;
	}
	//4. 检测文件大小
	fseek(pBmpFile, 0, SEEK_END);
	u32 dwFileSize = ftell(pBmpFile);
	if (dwFileSize <= 54) //54 = bmp file header(14) + bitmap information(40)
	{
		SAFE_FCLOSE(pBmpFile)
			MError("\"%s\" picSize is error.\n", pchFilePath);
		return -3;
	}
	//5. 分配文件缓冲
	fseek(pBmpFile, 0, SEEK_SET);
	u8* pData = new u8[dwFileSize];
	CHECK_NEW_PTR(pData);
	if (NULL == pData)
	{
		SAFE_FCLOSE(pBmpFile);
			MError("new u8[%d] failed.\n", dwFileSize);
		return -4;
	}
	//6. 判断文件是否为BMP图片
	fread(pData, 1, dwFileSize, pBmpFile);
	if (*(u16*)(pData) != 0x4d42) //bmp
	{
		SAFE_DELETE_ARRAY(pData);
		SAFE_FCLOSE(pBmpFile);
		MError("%s is not BMP.\n", pchFilePath);
		return -5;
	}
	//7. 检测图片是否为 RGB24 格式
	if (24 != BmpBitCount(pData)) //暂时只支持RGB24，后续可以增加格式
	{
		SAFE_DELETE_ARRAY(pData);
		SAFE_FCLOSE(pBmpFile);
		MError("%s is not 24 bit pic.\n", pchFilePath);
		return -6;
	}
	//8. 读取BMP像素数据，并转换成 I420数据
	//u32 dwOffsetBits = BmpOffsetBits(pData);
	u32 dwWidth = BmpWidth(pData);
	u32 dwHeight = BmpHeight(pData);
	ReInitRawYuvData(&tOutPutYuv, dwWidth*dwHeight*2);
	if (tOutPutYuv.pDataY != NULL)
	{
		tOutPutYuv.dwCompression = (u32)en_VideoI420;
		tOutPutYuv.pDataU = tOutPutYuv.pDataY + (dwWidth * dwHeight);
		tOutPutYuv.pDataV = tOutPutYuv.pDataY + (dwWidth * dwHeight)*5/4;
		tOutPutYuv.dwWidth = dwWidth;
		tOutPutYuv.dwHeight = dwHeight;
		rgb24_to_yv12_c11(tOutPutYuv.pDataY, tOutPutYuv.pDataU, tOutPutYuv.pDataV, pData + 54/*dwOffsetBits*/, dwWidth, dwHeight, dwWidth);
	}
	//9. 释放缓冲，关闭文件
	SAFE_DELETE_ARRAY(pData);
	SAFE_FCLOSE(pBmpFile);
	bReloadPic = FALSE; //BMP载入成功后，重载标志关闭
	return 0;
////////////////////////////////////////////////////////////////////
}

u32 BmpInfo( wchar_t* pchFilePath, u8* pBmpInfo, u32 dwSize)
{
	if (pBmpInfo == NULL || dwSize < 54 || pchFilePath == NULL)
	{
		MError("input param is invalid!\n");
		return -1;
	}
	u32 dwRet = -1;
	FILE* pBmpFile = _wfopen(pchFilePath,L"rb");
	if (pBmpFile != NULL)
	{
		fseek(pBmpFile, 0, SEEK_END);
		u32 dwFileSize = ftell(pBmpFile);
		if (dwFileSize > 54)    //54 = bmp file header(14) + bitmap information(40)
		{
			fseek(pBmpFile, 0, SEEK_SET);
			fread(pBmpInfo, 1, 54, pBmpFile);
			dwRet = 0;
		}
	}
	if (pBmpFile)
	{
		fclose(pBmpFile);
		pBmpFile = NULL;
	}
	return dwRet;
}

//u16 SaveRawDataToFile( TVidRawData *pYuvData, wchar_t* pchFilePath, u8 byEncodeMode )
//{
//	ERROR_CHECK(pYuvData)
//	ERROR_CHECK(pchFilePath)
//	ERROR_CHECK(pYuvData->pDataY)
//	//u32 dwType = 0;
//	//s32 nPathSize = strlen(pchFilePath);
//	//if (nPathSize > 5 && 
//	//	(pchFilePath[nPathSize - 4] == 'j'  ||  pchFilePath[nPathSize - 4] == 'J')
//	//	&& (pchFilePath[nPathSize - 3] == 'p'||  pchFilePath[nPathSize - 2] == 'P' )
//	//	&& (pchFilePath[nPathSize - 2] == 'e' ||  pchFilePath[nPathSize - 2] == 'E')
//	//	&& pchFilePath[nPathSize - 1] == 'g' || pchFilePath[nPathSize - 1] == 'G')
//	//{
//	//	dwType = 1;
//	//}
//	//else 
//	//{
//	//}
//	if (pYuvData->dwCompression != ((u32)en_VideoI420) || byEncodeMode == 0)
//	{
//		MError("Input param is invalid. pYuvData->dwCompression=%d(en_VideoI420=0),byEncodeMode=%d \n", pYuvData->dwCompression, byEncodeMode);
//		return -1;
//	}
//	else if (pYuvData->dwWidth == 0 || pYuvData->dwHeight == 0 || pYuvData->dwDataSize < pYuvData->dwWidth * pYuvData->dwHeight*3/2)
//	{
//		MError("Input param is invalid. WxH=%dx%d, pYuvData->dwDataSize=%d, WxHx1.5=%d\n", 
//			pYuvData->dwWidth, pYuvData->dwHeight, pYuvData->dwDataSize, pYuvData->dwWidth * pYuvData->dwHeight*3/2);
//		return -2;
//	}
//	else
//	{
//		if (pYuvData->pDataU == NULL || pYuvData->pDataV == NULL)
//		{//确保尺寸
//			pYuvData->pDataU = pYuvData->pDataY + pYuvData->dwWidth * pYuvData->dwHeight;
//			pYuvData->pDataV = pYuvData->pDataU + pYuvData->dwWidth * pYuvData->dwHeight/4;
//		}
//		u8 *pRGBData = new u8[pYuvData->dwWidth * pYuvData->dwHeight * 3];
//		CHECK_NEW_PTR(pRGBData)
//		if (pRGBData)
//		{
//			yv12_to_rgb24_1234(pRGBData, pYuvData->dwWidth,  pYuvData->pDataY, pYuvData->pDataU, pYuvData->pDataV, 
//				pYuvData->dwWidth, pYuvData->dwWidth >>1, pYuvData->dwWidth, pYuvData->dwHeight);
//			//保存的图像上下颠倒一次
//			u8* pTempAddrTop = pRGBData;
//			u8* pTempAddrBottom = pRGBData + pYuvData->dwWidth  * (pYuvData->dwHeight -1) * 3;
//			u32 dwLineSize = pYuvData->dwWidth  * 3;
//			u8* pTemp = new u8[dwLineSize];
//			CHECK_NEW_PTR(pTemp)
//			for (u32 nYos = 0; nYos < pYuvData->dwHeight/2; nYos++)
//			{
//				memcpy(pTemp, pTempAddrTop, dwLineSize);
//				memcpy(pTempAddrTop , pTempAddrBottom, dwLineSize);
//				memcpy(pTempAddrBottom, pTemp, dwLineSize);
//				pTempAddrTop += dwLineSize;
//				pTempAddrBottom -= dwLineSize;
//			}
//			SAFE_DELETE_ARRAY(pTemp)
//			if (byEncodeMode == 1)
//			{
//#if _MSC_VER <= 1600 // VS2010以下
//				int nRet = 0;
//				int nCnt = 0;
//				do{
//					nRet = RGB2JPGFile(pRGBData, pYuvData->dwWidth, pYuvData->dwHeight, pchFilePath);
//					nCnt++;
//					if (nCnt == 3)
//					{
//						break;
//					}
//				}while(nRet != 1);
//				MPrintf("JPG FilePath:%ls line:%d, nret:%d\n", pchFilePath, __LINE__, nRet);
//#elif _MSC_VER >= 1900 //vs2015
//
//				s8 achJpgName[MAX_FILE_PATH_LEN] = { 0 };
//				WC2MB(pchFilePath, achJpgName, MAX_FILE_PATH_LEN);
//				rgb2jpg(achJpgName, (char*)pRGBData, pYuvData->dwWidth, pYuvData->dwHeight);
//#endif				
//				
//			}
//			if (byEncodeMode == 2)
//			{
//
//				BITMAPINFOHEADER binfo;//bitmap info-header
//				memset(&binfo, 0 ,sizeof(BITMAPINFOHEADER));
//				binfo.biWidth = pYuvData->dwWidth;
//				binfo.biHeight = pYuvData->dwHeight;
//				binfo.biPlanes = 1;
//				binfo.biCompression = BI_RGB;
//				binfo.biBitCount = 24;
//				binfo.biSizeImage = pYuvData->dwWidth * pYuvData->dwHeight * 3;
//				binfo.biSize = sizeof(BITMAPINFOHEADER);
//				binfo.biClrUsed = 0;
//				binfo.biXPelsPerMeter = 0;
//				binfo.biYPelsPerMeter = 0;
//
//				BITMAPFILEHEADER hdr; //bitmap file-header
//				hdr.bfType = 0x4d42;  //0x4d = "M" 0x42 = "B"
//				hdr.bfSize = (u32)(sizeof(BITMAPFILEHEADER) + binfo.biSize + binfo.biClrUsed * sizeof(RGBQUAD) + binfo.biSizeImage);
//				hdr.bfReserved1 = 0;
//				hdr.bfReserved2 = 0;
//				hdr.bfOffBits = (u32)sizeof(BITMAPFILEHEADER) + binfo.biSize + binfo.biClrUsed * sizeof (RGBQUAD);
//
//				FILE *fSaveBmp = _wfopen(pchFilePath, L"wb");
//				if (fSaveBmp)
//				{
//					fwrite(&hdr, 1, sizeof(BITMAPFILEHEADER), fSaveBmp);
//					fwrite(&binfo, 1, binfo.biSize, fSaveBmp);	
//					fwrite(pRGBData, 1, binfo.biSizeImage, fSaveBmp);
//					fflush(fSaveBmp);
//				}
//				SAFE_FCLOSE(fSaveBmp);
//				MPrintf("Bmp FilePath:%ls \n", pchFilePath);
//			}
//		}
//		SAFE_DELETE_ARRAY(pRGBData)
//	}
//	return 0;
//}
s8* GetMediaTypeName(u32 dwMediaType)
{
	const s8* pMediaTypeName = NULL;
	switch (dwMediaType)
	{
	case MEDIA_TYPE_H261:
		pMediaTypeName = "31: H261";
		break;
	case MEDIA_TYPE_H262:
		pMediaTypeName = "33: H262 (MPEG-2)";
		break;
	case MEDIA_TYPE_H263:
		pMediaTypeName = "34: H263";
		break;
	case MEDIA_TYPE_H263PLUS:
		pMediaTypeName = "101: H263+";
		break;
	case MEDIA_TYPE_H264:
		pMediaTypeName = "106: H264";
		break;
	case MEDIA_TYPE_H264_ForHuawei:
		pMediaTypeName = "105: H264_HuaWei";
		break;
	case MEDIA_TYPE_H265:
		pMediaTypeName = "108: H265";
		break;
	case MEDIA_TYPE_MP4:
		pMediaTypeName = "97: MPEG-4";
		break;
	case MEDIA_TYPE_FEC:
		pMediaTypeName = "107: FEC";
		break;
	default:
		pMediaTypeName = "Unknown";	
	}
	return (s8*)pMediaTypeName;
}

s8* GetRawVideoTypeName(u32 dwRawVideoType)
{
	const s8* pRawVideoTypeName = NULL;
	switch (dwRawVideoType)
	{
	case en_VideoI420:
		pRawVideoTypeName = "0=en_VideoI420";
		break;
	case en_VideoYV12:
		pRawVideoTypeName = "1=en_VideoYV12";
		break;
	case en_VideoYUY2:
		pRawVideoTypeName = "2=en_VideoYUY2";
		break;
	case en_VideoUYVY:
		pRawVideoTypeName = "3=en_VideoUYVY";
		break;
	case en_VideoIYUV:
		pRawVideoTypeName = "4=en_VideoIYUV";
		break;
	case en_VideoARGB:
		pRawVideoTypeName = "5=en_VideoARGB";
		break;
	case en_VideoRGB24:
		pRawVideoTypeName = "6=en_VideoRGB24";
		break;
	case en_VideoRGB565:
		pRawVideoTypeName = "7=en_VideoRGB565";
		break;
	case en_VideoARGB4444:
		pRawVideoTypeName = "8=en_VideoARGB4444";
		break;
	case en_VideoARGB1555:
		pRawVideoTypeName = "9=en_VideoARGB1555";
		break;
	case en_VideoMJPEG:
		pRawVideoTypeName = "10=en_VideoMJPEG";
		break;
	case en_VideoNV12:
		pRawVideoTypeName = "11=en_VideoNV12";
		break;
	case en_VideoNV21:
		pRawVideoTypeName = "12=en_VideoNV21";
		break;
	case en_VideoBGRA:
		pRawVideoTypeName = "13=en_VideoBGRA";
		break;
	default:
		pRawVideoTypeName = "en_VideoUnknown";	
	}
	return (s8*)pRawVideoTypeName;
}

s8* GetCodecModeName(u8 byCoderMode)
{
	const s8* szEncoderType = "None";
	switch (byCoderMode)
	{
	case em_Codec_None:
		szEncoderType = "None";
		break;
	case em_Codec_HW:
		szEncoderType = "HW";
		break;
	case em_Codec_VFW:
		szEncoderType = "VFW";
		break;
	case em_Codec_X264:
		szEncoderType = "X264";
		break;
	case em_Codec_VideoUnit:
		szEncoderType = "VU";
		break;
	default:
		szEncoderType = "None";
		break;
	}
	return (s8*)szEncoderType;
}

//s8* GetDrawModeName(u8 byDrawMode)
//{
//	const s8* szDrawMode = "None";
//	switch (byDrawMode)
//	{
//	case DDRAW:
//		szDrawMode = "DDRAW";
//		break;
//	case GDI:
//		szDrawMode = "GDI";
//		break;
//	case DRAWDIB:
//		szDrawMode = "DRAWDIB";
//		break;
//	case D3D:
//		szDrawMode = "D3D";
//		break;
//	default:
//		szDrawMode = "None";
//		break;
//	}
//	return (s8*)szDrawMode;
//}

s8* GetVidCapTypeName(u8 byCapType)
{
	const s8* szCapType = "None";
	switch(byCapType)
	{
	case emUsbCamera:
		szCapType = "emUsbCamera";
		break;
	case emDeskTop:
		szCapType = "emDeskTop";
		break;
	case emDeskShared:
		szCapType = "emDeskShared";
		break;
	default:
		szCapType = "None";
		break;
	}
	return (s8*)szCapType;
}

//inline void PrintSysErr(s32 nErrorID)
//{
//	MError("[ErrID:%d] %s\n", nErrorID, strerror(nErrorID));
//};
