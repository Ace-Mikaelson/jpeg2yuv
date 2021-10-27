

PWD_LIBJPEG_TURBO=~/log/jpegtest/libjpeg-turbo-2.0.4









all: main.c
	gcc main.c -I $(PWD_LIBJPEG_TURBO)/build-arm/build/include $(PWD_LIBJPEG_TURBO)/build-arm/build/lib/libturbojpeg.a 


clean:
	rm -rf a.out target.yuv target.jpeg *.yuv *_*_*
