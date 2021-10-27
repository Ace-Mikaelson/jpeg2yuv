#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "turbojpeg.h"
#include <string.h>
#include <time.h>
struct coordinate
{
	int width; 			//img Original width
	int height; 		//img Original height
	int rect_x0; 		//img Starting coordinate system width
	int rect_y0; 		//img Starting coordinate system height
	int rect_width; 	//The width of the screenshot is required
	int rect_height; 	//The height of the screenshot is required
};

int dump_for_test(unsigned char *buffer, size_t size, char * strings)
{
	char file_name[128] = { 0 };	
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	//sprintf(file_name, "%04d_%02d_%02d_%02d_%02d_%02d", (1900 + p->tm_year),(1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
	sprintf(file_name, "%02d_%02d_%02d", p->tm_hour, p->tm_min, p->tm_sec);
	strcat(file_name, "_");
	strcat(file_name, strings);

	FILE *fp = fopen(file_name, "w");
	fseek( fp, 0, SEEK_END);
	fwrite( buffer, size, 1, fp );
	fclose( fp );
}

int matting_yuv420p(unsigned char *src, struct coordinate *src_cdn, unsigned char *dest, struct coordinate *dest_cdn)
{
	if( src_cdn->width % 2 || src_cdn->height % 2
		|| src_cdn->rect_x0 % 2 || src_cdn->rect_y0 % 2
		|| src_cdn->rect_height % 2 || src_cdn->rect_width % 2 
		|| dest_cdn->width % 2 || dest_cdn->height % 2
		|| dest_cdn->rect_x0 % 2 || dest_cdn->rect_y0 % 2
		|| dest_cdn->rect_width % 2 || dest_cdn->rect_height % 2 
		|| src_cdn->rect_x0 > src_cdn->width || src_cdn->rect_width > src_cdn->width
		|| src_cdn->rect_y0 > src_cdn->height|| src_cdn->rect_height > src_cdn->height
		|| src_cdn->rect_width > dest_cdn->width || dest_cdn->rect_x0 > dest_cdn->width
		|| src_cdn->rect_height > dest_cdn->height || dest_cdn->rect_y0 > dest_cdn->height) {
		
		printf("fail input param is error\n");
		return -1;
	}

	if( src_cdn->rect_x0 + src_cdn->rect_width > src_cdn->width ) {
		src_cdn->rect_width = src_cdn->width - src_cdn->rect_x0;
	} else if ( src_cdn->rect_y0 + src_cdn->rect_height > src_cdn->height ) {
		src_cdn->rect_height = src_cdn->height = src_cdn->rect_y0;
	} else if ( src_cdn->rect_width + dest_cdn->rect_x0 > dest_cdn->width ) {
		src_cdn->rect_width = dest_cdn->width - dest_cdn->rect_x0;
	} else if ( src_cdn->rect_height + dest_cdn->rect_y0 > dest_cdn->height ) {
		src_cdn->rect_height = dest_cdn->height - dest_cdn->rect_y0;
	}


	int i = 0;
	int j = 0;
	unsigned char *pU_src = src + src_cdn->width * src_cdn->height;
	unsigned char *pV_src = src + src_cdn->width * src_cdn->height * 5 / 4;
	unsigned char *pU_dest = dest + dest_cdn->width * dest_cdn->height;
	unsigned char *pV_dest = dest + dest_cdn->width * dest_cdn->height * 5 / 4;

	for( i = src_cdn->rect_y0 ; i < src_cdn->rect_height + src_cdn->rect_y0 ; i++ )
	{
		for( j = src_cdn->rect_x0 ; j < src_cdn->rect_width + src_cdn->rect_x0 ; j++ )
		{
			* ( dest + ( i - src_cdn->rect_y0 + dest_cdn->rect_y0) * dest_cdn->width + ( j - src_cdn->rect_x0 + dest_cdn->rect_x0) ) = *(src + i * src_cdn->width + j );
			if( (1 == (j % 2)) && (1 == (i % 2)) )
			{
				* ( pU_dest + ((i - src_cdn->rect_y0 + dest_cdn->rect_y0) >> 1 ) * (dest_cdn->width >> 1) + ((j - src_cdn->rect_x0 + dest_cdn->rect_x0) >> 1) ) = * ( pU_src + ( i >> 1 ) * ( src_cdn->width >> 1 ) + ( j >> 1 ) ); 
				* ( pV_dest + ((i - src_cdn->rect_y0 + dest_cdn->rect_y0) >> 1 ) * (dest_cdn->width >> 1) + ((j - src_cdn->rect_x0 + dest_cdn->rect_x0) >> 1) ) = * ( pV_src + ( i >> 1 ) * ( src_cdn->width >> 1 ) + ( j >> 1 ) );
			}
		}
	}
	return 0;
}



int create_background_yuv420p(unsigned char **background_buffer, int background_width, int background_height)
{

	int background_size = background_width * background_height * 3 / 2;
	memset( *background_buffer, 0, background_width * background_height);

	unsigned char *background_u = *background_buffer + background_width * background_height;
	memset(background_u, 128, background_width * background_height >> 2);
	unsigned char *background_v = background_u + ((background_width * background_height) >> 2);
	memset(background_v, 128, background_width * background_height >> 2);
	return 0;
}



int convert_JPEG_to_I420(unsigned char* jpeg_buffer, int jpeg_size, unsigned char* yuv_buffer, int* yuv_size, int* yuv_type)
{
    tjhandle handle = NULL;
    int width, height, subsample, colorspace;
    int flags = 0;
    int padding = 1; // 1或4均可，但不能是0
    int ret = 0;
 
    handle = tjInitDecompress();
    tjDecompressHeader3(handle, jpeg_buffer, jpeg_size, &width, &height, &subsample, &colorspace);
 
    printf("w: %d h: %d subsample: %d color: %d\n", width, height, subsample, colorspace);
    
    flags |= 0;
    
    *yuv_type = subsample;
    // 注：经测试，指定的yuv采样格式只对YUV缓冲区大小有影响，实际上还是按JPEG本身的YUV格式来转换的
    *yuv_size = tjBufSizeYUV2(width, padding, height, subsample);
 
    ret = tjDecompressToYUV2(handle, jpeg_buffer, jpeg_size, yuv_buffer, width,
			padding, height, flags);
    if (ret < 0)
    {
        printf("compress to jpeg failed: %s\n", tjGetErrorStr());
    }
    tjDestroy(handle);
 
    return ret;
}

int convert_YUV_to_JPEG(unsigned char* yuv_buffer, int yuv_size, int width, int height, int subsample, unsigned char** jpeg_buffer, unsigned long* jpeg_size, int quality)
{
    tjhandle handle = NULL;
    int flags = 0;
    int padding = 1; // 1或4均可，但不能是0
    int need_size = 0;
    int ret = 0;
 
    handle = tjInitCompress();
   
    flags |= 0;
 
    need_size = tjBufSizeYUV2(width, padding, height, subsample);
    if (need_size != yuv_size)
    {
        printf("we detect yuv size: %d, but you give: %d, check again.\n", need_size, yuv_size);
        return 0;
    }
 
    ret = tjCompressFromYUV(handle, yuv_buffer, width, padding, height, subsample, jpeg_buffer, jpeg_size, quality, flags);
    if (ret < 0)
    {
        printf("compress to jpeg failed: %s\n", tjGetErrorStr());
    }
 
    tjDestroy(handle);
 
    return ret;
}





int main(int argc, const char *argv[])
{
	int img_width = 320;
	int img_height = 320;
	int yuv_size = 0;
	int yuv_type = 0;

	int background_width = 1280;
	int background_height = 720;
	unsigned long int jpeg_size = 0;
	unsigned char *jpeg_buffer = NULL;
	unsigned char *yuv_buffer = NULL;
	size_t jpeg_Read = 0;

	FILE *jpeg_fp = NULL;



	/* 读取 jpeg数据 */
	jpeg_fp = fopen("test.jpeg", "rb");
	if( NULL == jpeg_fp ){
		printf("fail to fopen ");
		return -1;
	}
	fseek(jpeg_fp, 0, SEEK_END);
	jpeg_size = ftell( jpeg_fp );
	fseek(jpeg_fp, 0, SEEK_SET);
	jpeg_buffer = malloc( jpeg_size );
	if( jpeg_buffer == NULL ){
		printf("fail to malloc jpeg_buffer");
		return -1;
	}
	jpeg_Read = fread( jpeg_buffer, 1, jpeg_size, jpeg_fp);
	if(jpeg_Read != jpeg_size)
	{
		printf("fail jpeg_size is not match jpeg_Read");
		return -1;
	}
	printf("jpeg_size = %ld , jpeg_Read = %ld\n",jpeg_size , jpeg_Read);



	/* 分配yuv bufer */
	yuv_buffer = malloc(sizeof(char) * img_width * img_height * 3 / 2 );
	if( NULL == yuv_buffer ) {
		printf("fail to malloc yuv_buffer");
		return -1;
	}



	/* 分配黑色背景buffer  */
	unsigned char * background_buffer = (unsigned char *)malloc(sizeof(char) * background_width * background_height * 3 / 2);
	if( NULL == background_buffer ) {
		printf("fail to malloc background_buffer");
		return -1;
	}



	convert_JPEG_to_I420(jpeg_buffer, jpeg_size, yuv_buffer, &yuv_size, &yuv_type);
	create_background_yuv420p( &background_buffer, background_width, background_height );


#if 0
	memset(jpeg_buffer, 0, jpeg_size);
	convert_YUV_to_JPEG(yuv_buffer, yuv_size, img_width, img_height, yuv_type, &jpeg_buffer, &jpeg_size, 95);
	dump_for_test( jpeg_buffer, jpeg_size, "target.jpeg");
#endif
#if 0
	dump_for_test(yuv_buffer, yuv_size, "before.yuv");
#endif




	/*抠图*/
	struct coordinate yuv_cdn;
	yuv_cdn.width = img_width;
	yuv_cdn.height = img_height;
	yuv_cdn.rect_x0 = 50;
	yuv_cdn.rect_y0 = 50;
	yuv_cdn.rect_width = 200;
	yuv_cdn.rect_height = 200;	
	struct coordinate background_cdn;
	background_cdn.width = background_width;
	background_cdn.height = background_height;
	background_cdn.rect_x0 = 100;
	background_cdn.rect_y0 = 100;
	background_cdn.rect_width = background_cdn.rect_x0 + yuv_cdn.rect_width;
	background_cdn.rect_height = background_cdn.rect_y0 + yuv_cdn.rect_height;
	matting_yuv420p(yuv_buffer, &yuv_cdn, background_buffer, &background_cdn);
	dump_for_test(background_buffer, background_width * background_height * 3 / 2, "after_test.yuv");




	if( background_buffer ){
		free(background_buffer);
		background_buffer = NULL;
	}
	if(yuv_buffer){
		free(yuv_buffer);
		yuv_buffer = NULL;
	}
	if( jpeg_buffer ){
		free( jpeg_buffer );
		jpeg_buffer = NULL;
	}
	fclose( jpeg_fp );
	return 0;
}
