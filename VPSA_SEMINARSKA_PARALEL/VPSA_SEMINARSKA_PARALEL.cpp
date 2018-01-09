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
#define MAXITER 301
void poisciSive(int h, int w,int* slikaInput,int* slikaInputOriginal);
int main(void)
{
	double start_omp = omp_get_wtime();
	int max_iter = MAXITER;
	int st_niti = WORKGROUP_SIZE;
	//Za enkat delam z predpostavko da ze mam sliko poracunano s sobelom

	//Zapomnemo si originalno sliko
	PGMData slikaInputOriginal;
	readPGM("towerSobel_back.pgm", &slikaInputOriginal);

	//Alociramo prostor tudi za sliko iz katere bomo rezali ven
	PGMData slikaKoncna;
	slikaKoncna.width = slikaInputOriginal.width;
	slikaKoncna.height = slikaInputOriginal.height;
	slikaKoncna.max_gray = 255;
	slikaKoncna.image = (int *)malloc(slikaInputOriginal.height*(slikaInputOriginal.width) * sizeof(int));

	//Alociramo prostor za sliko ki jo bomo obdelovali
	PGMData slikaInput;
	readPGM("towerSobel_back.pgm", &slikaInput);

	//Potrebna se nakonc zapisat v 
	//writePGM("towerSobelFinal.pgm", &slikaKoncna);


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

	// Delitev dela
	size_t local_item_size = WORKGROUP_SIZE;
	size_t global_item_size = local_item_size;

	// Alokacija pomnilnika na napravi, ALOKACIJA ZA SLIKA INPUT
	cl_mem mem_obj_original = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		slikaInputOriginal.width*slikaInputOriginal.height * sizeof(int), slikaInputOriginal.image, &ret);

	cl_mem mem_obj_input = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		slikaInput.width*slikaInput.height * sizeof(int), slikaInput.image, &ret);

	cl_mem mem_obj_koncna = clCreateBuffer(context, CL_MEM_WRITE_ONLY ,
		slikaKoncna.width*slikaKoncna.height * sizeof(int), NULL, &ret);


	// Priprava programa
	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str,
		NULL, &ret);
	// kontekst, "stevilo kazalcev na kodo, kazalci na kodo,		
	// stringi so NULL terminated, napaka													

	// Prevajanje
	ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);
	// program, "stevilo naprav, lista naprav, opcije pri prevajanju,

	// Log
	/*
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
	*/

	// "s"cepec: priprava objekta
	cl_kernel kernel = clCreateKernel(program, "seamCarving", &ret);
	// program, ime "s"cepca, napaka

	size_t buf_size_t;
	clGetKernelWorkGroupInfo(kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(buf_size_t), &buf_size_t, NULL);

	//ZACETEK ZAKE
	
	for (int i = 0; i < MAXITER; i++) {
		int w = slikaInput.width;
		int h = slikaInput.height;
		ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&mem_obj_input);
		ret |= clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&slikaInput.width);
		ret |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&slikaInput.height);

		//ret |= clSetKernelArg(kernel, 9, sizeof(cl_int), (void *)&max_iter);
		// "s"cepec, "stevilka argumenta, velikost podatkov, kazalec na podatke
		//printf("%d",ret);
		// "s"cepec: zagon
		ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
			&global_item_size, &local_item_size, 0, NULL, NULL);
		// vrsta, "s"cepec, dimenzionalnost, mora biti NULL, 
		// kazalec na "stevilo vseh niti, kazalec na lokalno "stevilo niti, 
		// dogodki, ki se morajo zgoditi pred klicem
		//slikaKoncna.width = slikaKoncna.width - MAXITER;
		ret = clEnqueueReadBuffer(command_queue, mem_obj_input, CL_TRUE, 0, slikaInput.width * slikaInput.height * sizeof(int), slikaInput.image, 0, NULL, NULL);
		ret = clFlush(command_queue);
		//Sedaj imamo v slikaInput poračunane vse vsote, gremo gledati minimum
		poisciSive(h, w, slikaInput.image, slikaInputOriginal.image);
		//Sedaj imamo postavljene vrednosti na -1, tam kjer smo imeli najmanše vsote, te bomo izrezali

		//Width-1, ker režemo po širini
		slikaKoncna.width = slikaInputOriginal.width - 1;
		slikaKoncna.height = slikaInputOriginal.height;
		//Fun fact, če sem imel for od 0 do velikosti slike, jo je izrisalo obratno, če to delam obratno je pravilno
		int cnt = slikaInputOriginal.height*(slikaInputOriginal.width - 1);
		for (int i = slikaInputOriginal.height*slikaInputOriginal.width; i>0; i--) {
			if (slikaInputOriginal.image[i] != -1) {
				slikaKoncna.image[cnt] = slikaInputOriginal.image[i];
				cnt--;
			}
		}
		//Posodobim slikaInputOriginal, ki mi hrani vrednosti za rezanje
		slikaInputOriginal.height = slikaKoncna.height;
		slikaInputOriginal.width = slikaKoncna.width;
		memcpy(slikaInputOriginal.image, slikaKoncna.image, slikaKoncna.width*slikaKoncna.height * sizeof(int));

		//Posodobim slikaInput, ki mi hrani vsote
		slikaInput.height = slikaKoncna.height;
		slikaInput.width = slikaKoncna.width;
		memcpy(slikaInput.image, slikaKoncna.image, slikaKoncna.width*slikaKoncna.height * sizeof(int));

		ret = clEnqueueWriteBuffer(command_queue, mem_obj_input, CL_TRUE, 0, slikaInput.width * slikaInput.height * sizeof(int), slikaInput.image, 0, NULL, NULL);
		ret = clFlush(command_queue);
	}




	

	// zadnji trije - dogodki, ki se morajo zgoditi prej

	// Prikaz rezultatov);

	// "ci"s"cenje
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(mem_obj_input);
	ret = clReleaseMemObject(mem_obj_koncna);
	ret = clReleaseMemObject(mem_obj_original);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	writePGM("boi.pgm", &slikaKoncna);
	printf("Total run time: %g", omp_get_wtime()- start_omp);

	return 0;
}


void poisciSive(int h, int w, int* slikaInput,int* slikaInputOriginal) {

	int najmanjsa_vrednost_v_vrstici = INT_MAX;
	int indeks_najmanjse_vrednosti_v_vrstici = -1;
	//Rabimo pogledati vse vrednosti samo za prvo vrstico
	for (int i = 0; i<w;i++) {
		if (slikaInput[i]<najmanjsa_vrednost_v_vrstici) {
			najmanjsa_vrednost_v_vrstici = slikaInput[i];
			indeks_najmanjse_vrednosti_v_vrstici = i;
		}
	}
	slikaInput[indeks_najmanjse_vrednosti_v_vrstici] = -1;
	//Začnemo v prvi vrstici (ne v ničti, tam smo iskali prvotno vrednost)
	int trenutna_vrstica = 1;
	while (trenutna_vrstica<h) {
		//če je trenutni indeks % w enak širini pomeni da smo na začetku, pregledamo le 2 elementa
		if (indeks_najmanjse_vrednosti_v_vrstici%w == w) {
			if (slikaInput[indeks_najmanjse_vrednosti_v_vrstici + w]<slikaInput[indeks_najmanjse_vrednosti_v_vrstici + w + 1]) {
				najmanjsa_vrednost_v_vrstici = slikaInput[indeks_najmanjse_vrednosti_v_vrstici + w];
				indeks_najmanjse_vrednosti_v_vrstici = indeks_najmanjse_vrednosti_v_vrstici + w;
			}
			else {
				najmanjsa_vrednost_v_vrstici = slikaInput[indeks_najmanjse_vrednosti_v_vrstici + w + 1];
				indeks_najmanjse_vrednosti_v_vrstici = indeks_najmanjse_vrednosti_v_vrstici + w + 1;
			}
		}
		//če je trenutni indeks % w enak širini -1 pomeni da smo na robu, pregledamo le 2 elementa
		else if (indeks_najmanjse_vrednosti_v_vrstici%w == w - 1) {
			if (slikaInput[indeks_najmanjse_vrednosti_v_vrstici + w]<slikaInput[indeks_najmanjse_vrednosti_v_vrstici - 1]) {
				najmanjsa_vrednost_v_vrstici = slikaInput[indeks_najmanjse_vrednosti_v_vrstici + w];
				indeks_najmanjse_vrednosti_v_vrstici = indeks_najmanjse_vrednosti_v_vrstici + w;
			}
			else {
				najmanjsa_vrednost_v_vrstici = slikaInput[indeks_najmanjse_vrednosti_v_vrstici + w - 1];
				indeks_najmanjse_vrednosti_v_vrstici = indeks_najmanjse_vrednosti_v_vrstici + w - 1;
			}
		}
		//Nismo ne levo ne desno ob koncu, smo nekje na sredini tako da gledamo 3 sosede
		else {
			int a = slikaInput[indeks_najmanjse_vrednosti_v_vrstici + (w - 1)];
			int b = slikaInput[indeks_najmanjse_vrednosti_v_vrstici + w];
			int c = slikaInput[indeks_najmanjse_vrednosti_v_vrstici + (w + 1)];
			//Preverim levega
			if (a<b && a<c) {
				najmanjsa_vrednost_v_vrstici = a;
				indeks_najmanjse_vrednosti_v_vrstici = indeks_najmanjse_vrednosti_v_vrstici + (w - 1);
			}
			//Preverim srednjega
			else if (b<a && b<c) {
				najmanjsa_vrednost_v_vrstici = b;
				indeks_najmanjse_vrednosti_v_vrstici = indeks_najmanjse_vrednosti_v_vrstici + (w);
			}
			//Preverim desnega
			else {
				najmanjsa_vrednost_v_vrstici = c;
				indeks_najmanjse_vrednosti_v_vrstici = indeks_najmanjse_vrednosti_v_vrstici + (w + 1);
			}
		}
		slikaInput[indeks_najmanjse_vrednosti_v_vrstici] = -1;
		slikaInputOriginal[indeks_najmanjse_vrednosti_v_vrstici] = -1;
		trenutna_vrstica++;
	}
}