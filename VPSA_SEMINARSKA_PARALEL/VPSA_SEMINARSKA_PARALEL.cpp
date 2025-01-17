#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <CL/cl.h>
#include "pgm.h"
#include <omp.h>
#define SIZE			(1024)
#define WORKGROUP_SIZE	(512)
#define MAX_SOURCE_SIZE	16384 
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS 
#define MAXITER 500
int main(void)
{
	PGMData originalSlika;
	PGMData outputSlika;
	PGMData koncnaSlika;
	readPGM("towerpgm.pgm",&originalSlika);
	int imageSize = originalSlika.width*originalSlika.height;
	double start_omp = omp_get_wtime();
	outputSlika.image = (int *)malloc(originalSlika.height * originalSlika.width * sizeof(int));
	outputSlika.height = originalSlika.height;
	outputSlika.width = originalSlika.width;
	outputSlika.max_gray = originalSlika.max_gray;

	koncnaSlika.image = (int *)malloc(originalSlika.height * originalSlika.width * sizeof(int));
	koncnaSlika.height = outputSlika.height;
	koncnaSlika.width = outputSlika.width;
	koncnaSlika.max_gray = outputSlika.max_gray;

	int max_iter = MAXITER;
	int st_niti = WORKGROUP_SIZE;
	//Za enkat delam z predpostavko da ze mam sliko poracunano s sobelom


	char ch;
	int i;
	cl_int ret;

	// Branje datoteke
	FILE *fp;
	char *source_str;
	size_t source_size;

	fp = fopen("kernel.c", "r");
	if (!fp)
	{
		fprintf(stderr, ":-(#\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	source_str[source_size] = '\0';
	fclose(fp);

	// Rezervacija pomnilnika


	// Podatki o platformi
	cl_platform_id	platform_id[10];
	cl_uint			ret_num_platforms;
	char			*buf;
	size_t			buf_len;
	ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);
	// max. "stevilo platform, kazalec na platforme, dejansko "stevilo platform

	// Podatki o napravi
	cl_device_id	device_id[10];
	cl_uint			ret_num_devices;
	// Delali bomo s platform_id[0] na GPU
	ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 10,
		device_id, &ret_num_devices);
	// izbrana platforma, tip naprave, koliko naprav nas zanima
	// kazalec na naprave, dejansko "stevilo naprav

	// Kontekst
	cl_context context = clCreateContext(NULL, 1, &device_id[0], NULL, NULL, &ret);
	// kontekst: vklju"cene platforme - NULL je privzeta, "stevilo naprav, 
	// kazalci na naprave, kazalec na call-back funkcijo v primeru napake
	// dodatni parametri funkcije, "stevilka napake

	// Ukazna vrsta

	cl_command_queue command_queue = clCreateCommandQueue(context, device_id[0], 0, &ret);
	// kontekst, naprava, INORDER/OUTOFORDER, napake

	//Delitev dela za sobela
	size_t local_size_sobel[] = { 22, 22 };
	size_t group_count_sobel[] = { (originalSlika.width - 1) / 22 + 1, (originalSlika.height - 1) / 22 + 1 };
	size_t global_size_sobel[] = { group_count_sobel[0] * 22, group_count_sobel[1] * 22 };

	// Delitev dela
	size_t local_item_size_kumulative = WORKGROUP_SIZE;
	size_t global_item_size_kumulative = local_item_size_kumulative;

	
	//Alokacija pomnilika za sobela
	cl_mem input_mem_obj_sobel = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, imageSize * sizeof(int), originalSlika.image, &ret);
	cl_mem output_mem_obj_sobel = clCreateBuffer(context, CL_MEM_READ_WRITE, imageSize * sizeof(int), NULL, &ret);
	cl_mem koncen_mem_object = clCreateBuffer(context, CL_MEM_READ_WRITE, imageSize * sizeof(int), NULL, &ret);
	cl_mem siv_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, originalSlika.height * sizeof(int), NULL, &ret);

	// Priprava programa
	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str,
		NULL, &ret);
	// kontekst, "stevilo kazalcev na kodo, kazalci na kodo,		
	// stringi so NULL terminated, napaka													

	// Prevajanje
	ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);
	// program, "stevilo naprav, lista naprav, opcije pri prevajanju,

	// Log
	
	size_t build_log_len;
	char *build_log;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG,
		0, NULL, &build_log_len);
	// program, "naprava, tip izpisa, 
	// maksimalna dol"zina niza, kazalec na niz, dejanska dol"zina niza
	build_log = (char *)malloc(sizeof(char)*(build_log_len + 1));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG,
		build_log_len, build_log, NULL);
	printf("%s\n", build_log);
	free(build_log);
	

	// "s"cepec: priprava objekta
	
	//size_t buf_size_t;
	//clGetKernelWorkGroupInfo(kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(buf_size_t), &buf_size_t, NULL);
	//ret = clEnqueueReadBuffer(command_queue, output_mem_obj_sobel, CL_TRUE, 0, imageSize * sizeof(int), outputSlika.image, 0, NULL, NULL);
	//ZACETEK ZAKE
	cl_kernel kernel_sobel = clCreateKernel(program, "sobelGPU", &ret);
	cl_kernel kernel_kumulative = clCreateKernel(program, "seamCarving", &ret);
	cl_kernel kernel_siv = clCreateKernel(program, "odstraniSiv", &ret);

	for (int i = 0; i < MAXITER; i++) {
	// program, ime "s"cepca, napaka

	//kernel za sobelGPU
		
		ret = clSetKernelArg(kernel_sobel, 0, sizeof(cl_mem), (void *)&input_mem_obj_sobel);
		ret |= clSetKernelArg(kernel_sobel, 1, sizeof(cl_mem), (void *)&output_mem_obj_sobel);
		ret |= clSetKernelArg(kernel_sobel, 2, sizeof(cl_int), (void *)&originalSlika.width);
		ret |= clSetKernelArg(kernel_sobel, 3, sizeof(cl_int), (void *)&originalSlika.height);

		ret = clEnqueueNDRangeKernel(command_queue, kernel_sobel, 2, NULL,
			global_size_sobel, local_size_sobel, 0, NULL, NULL);

		
		ret = clSetKernelArg(kernel_kumulative, 0, sizeof(cl_mem), (void *)&output_mem_obj_sobel);
		ret |= clSetKernelArg(kernel_kumulative, 1, sizeof(cl_int), (void *)&originalSlika.width);
		ret |= clSetKernelArg(kernel_kumulative, 2, sizeof(cl_int), (void *)&originalSlika.height);
		ret |= clSetKernelArg(kernel_kumulative, 3, sizeof(cl_mem), (void *)&siv_mem_obj);


		ret = clEnqueueNDRangeKernel(command_queue, kernel_kumulative, 1, NULL,
			&global_item_size_kumulative, &local_item_size_kumulative, 0, NULL, NULL);
		
		originalSlika.width--;

		ret = clSetKernelArg(kernel_siv, 0, sizeof(cl_mem), (void *)&input_mem_obj_sobel);
		ret |= clSetKernelArg(kernel_siv, 1, sizeof(cl_mem), (void *)&koncen_mem_object);
		ret |= clSetKernelArg(kernel_siv, 2, sizeof(cl_mem), (void *)&siv_mem_obj);
		ret |= clSetKernelArg(kernel_siv, 3, sizeof(cl_int), (void *)&originalSlika.width);
		ret |= clSetKernelArg(kernel_siv, 4, sizeof(cl_int), (void *)&originalSlika.height);

		ret = clEnqueueNDRangeKernel(command_queue, kernel_siv, 2, NULL,
			global_size_sobel, local_size_sobel, 0, NULL, NULL);
		koncnaSlika.width--;

		cl_mem temp = input_mem_obj_sobel;
		input_mem_obj_sobel = koncen_mem_object;
		koncen_mem_object = temp;
		
	}
	ret = clEnqueueReadBuffer(command_queue, input_mem_obj_sobel, CL_TRUE, 0, koncnaSlika.width*koncnaSlika.height * sizeof(int), koncnaSlika.image, 0, NULL, NULL);

	//ret = clEnqueueReadBuffer(command_queue, output_mem_obj_sobel, CL_TRUE, 0, koncnaSlika.width*koncnaSlika.height * sizeof(int), koncnaSlika.image, 0, NULL, NULL);





	

	// zadnji trije - dogodki, ki se morajo zgoditi prej

	// Prikaz rezultatov);

	// "ci"s"cenje
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel_sobel);
	ret = clReleaseKernel(kernel_kumulative);
	ret = clReleaseProgram(program);
	//ret = clReleaseMemObject(mem_obj_input);
	//ret = clReleaseMemObject(mem_obj_koncna);
	//ret = clReleaseMemObject(mem_obj_original);
	ret = clReleaseMemObject(input_mem_obj_sobel);
	ret = clReleaseMemObject(output_mem_obj_sobel);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);
	/*
	int max_value = 0;
	
	for (int i = 0; i<koncnaSlika.width*koncnaSlika.height; i++) {
		if (koncnaSlika.image[i] > max_value){
			max_value = koncnaSlika.image[i];
		}
	}
	for (int i = 0; i<koncnaSlika.width*koncnaSlika.height; i++) {
		koncnaSlika.image[i] =(int)((float)koncnaSlika.image[i] / max_value * 255);
	}
	*/
	writePGM("para500iter_fixinbigger.pgm", &koncnaSlika);
	printf("Total run time: %g", omp_get_wtime()- start_omp);

	return 0;
}