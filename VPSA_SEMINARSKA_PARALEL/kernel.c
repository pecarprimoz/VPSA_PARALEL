#define NUM_THREADS 512
int return_global_id(int x, int y, int width);

static inline int returnMinimumValue(int a, int b, int c);
static inline void poisciSive(int h, int w, __global int* slikaInput, __global int* siv);
static inline int returnMinimumValue(int a, int b, int c){
if (a<b && a<c) {
		return a;
	}
	else if (b<a && b<c) {
		return b;
	}
	return c;
}
__kernel void seamCarving(__global int *slikaInput, int InputW, int InputH, __global int* siv)
{
	int vector_length_for_thread;
	int w;
	int h;
	w = InputW;
	h = InputH;

	vector_length_for_thread = (w - 1) / NUM_THREADS +1;
	int left;
	int middle;
	int right;
	for (int i = h - 2; i >= 0; i--) {
		for (int j = 0; j < vector_length_for_thread; j++) {
			//gremo po celotni vrstici
			//trenutna pozicija glede na širino in stolpec v katerem smo plus trenutni indeks 
			if (j + (get_global_id(0)*vector_length_for_thread) > w) {
				continue;
			}
			int trenutna_pozicija_v_vektorju = (w*i) + j + (get_global_id(0)*vector_length_for_thread);
			int trenutna_vrednost_v_vektorju = slikaInput[trenutna_pozicija_v_vektorju] ;
			left = slikaInput[trenutna_pozicija_v_vektorju + (w - 1)];
			middle = slikaInput[trenutna_pozicija_v_vektorju + (w)];
			right = slikaInput[trenutna_pozicija_v_vektorju + (w + 1)];
			//ROBNI PRIMER 1, CE SEM NA ZACETKU NE PREVERJAM LEVEGA
			if (j == 0) {
				//ÈE JE MOJ SPODNJI SOSED DIREKTNO POD MANO MANJŠI OD MOJEGA DESNEGA SOSEDA GA PRIŠTEJEM
				if (middle < right) {
					trenutna_vrednost_v_vektorju += middle;
				}
				else {
					trenutna_vrednost_v_vektorju += right;
				}
			}
			//ROBNI PRIMER 2, CE SEM NA KONCU NE PREVERJAM DESNEGA
			else if (j == w - 1) {
				if (middle < left) {
					trenutna_vrednost_v_vektorju += middle;
				}
				else {
					trenutna_vrednost_v_vektorju += left;
				}
			}
			//NI ROBNEGA PRIMERA, GLEDAM 3 SPODNJE SOSEDE
			else {
				trenutna_vrednost_v_vektorju += returnMinimumValue(left, middle, right);
			}
			//Posodobimo trenutno sliko
			
			slikaInput[trenutna_pozicija_v_vektorju] = trenutna_vrednost_v_vektorju;
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
	}
	if (get_global_id(0) == 0) {
		poisciSive(h, w, slikaInput, siv);
	}
}

static inline void updatePixelPos(__global int *input, __local int *shared, int x, int y,
	int width, int height,
	int l_x, int l_y,
	int shared_size,
	int group_x, int group_y,
	int g_idx, int l_idx) {
	/* GLEDAMO PO SLEDEČI DIMENZIJI
	TOP LEFT	UP, DOLOCA l_y == 0    TOP RIGHT
	|
	|
	LEFT l_x == 0	---------	       RIGHT l_x +1
	|
	|
	BOTTOM LEFT	 DOWN, DOLOCA l_y + 1	BOTTOM RIGHT
	DIMENZIJA Y ODVISNA OD GROUP_Y (VERTIKALNO)
	DIMENZIJA X ODVISNA OD GROUP_X (HORIZONTALNO)
	OSNOVNE DIMENZIJE GOR,DOL,LEVO,DESNO, UPOŠTEVAMO ŠE TOP LEFT, TOP RIGHT
	BOTTOM LEFT, BOTTOM RIGHT

	TOP LEFT --> l_x = 0, l_y = 0
	TOP RIGHT --> l_x + 1, l_y = 0
	BOTTOM LEFT l_x = 0, l_y +1
	BOTTOM RIGHT l_x + 1, l_y + 1

	GLEDE NA GLOBALNI ZAPOREDNI INDEKS POSODABLJAMO LOKALNO SKUPINO, tako mamo l_idx->g_idx
	posodabljamo lokalni pomnilnik glede na globalnega

	*/

	// left
	shared[l_idx] = input[g_idx]; // Trenutni pixel
	if (l_x == 0) {
		if (x == 0)
			shared[l_idx - 1] = 0;
		else
			shared[l_idx - 1] = input[g_idx - 1];
	}

	// right
	if (l_x + 1 == group_x) {
		if (x + 1 == width)
			shared[l_idx + 1] = 0;
		else
			shared[l_idx + 1] = input[g_idx + 1];
	}

	// top
	if (l_y == 0) {
		if (y == 0)
			shared[l_idx - shared_size] = 0;
		else
			shared[l_idx - shared_size] = input[g_idx - width];
	}

	// bottom
	if (l_y + 1 == group_y) {
		if (y + 1 == height)
			shared[l_idx + shared_size] = 0;
		else
			shared[l_idx + shared_size] = input[g_idx + width];
	}

	// top left
	if (l_x == 0 && l_y == 0) {
		if (x == 0 || y == 0)
			shared[l_idx - 1 * shared_size - 1] = 0;
		else
			shared[l_idx - 1 * shared_size - 1] = input[g_idx - 1 * width - 1];
	}

	// bottom left
	if (l_x == 0 && l_y + 1 == group_y) {
		if (x == 0 || y + 1 == height)
			shared[l_idx + 1 * shared_size - 1] = 0;
		else
			shared[l_idx + 1 * shared_size - 1] = input[g_idx + 1 * width - 1];
	}

	// top right
	if (l_x + 1 == group_x && l_y == 0) {
		if (x + 1 == width || y == 0)
			shared[l_idx - 1 * shared_size + 1] = 0;
		else
			shared[l_idx - 1 * shared_size + 1] = input[g_idx - 1 * width + 1];
	}

	// bottom right
	if (l_x + 1 == group_x && l_y + 1 == group_y) {
		if (x + 1 == width || y + 1 == height)
			shared[l_idx + 1 * shared_size + 1] = 0;
		else
			shared[l_idx + 1 * shared_size + 1] = input[g_idx + 1 * width + 1];
	}
	barrier(CLK_LOCAL_MEM_FENCE);
}


__kernel void sobelGPU(
	__global int *input,
	__global int *output,
	int width,
	int height)
{
	int Gx, Gy, tempPixel;

	int shared_size = 4 * 4;

	__local int shared[4 * 4];

	//trenutne globalne koordinate od niti
	int x = get_global_id(0);
	int y = get_global_id(1);

	//lokalne koordinate, v katerih bomo posodabljali
	size_t l_x = get_local_id(0);
	size_t l_y = get_local_id(1);

	// velikost skupine
	const size_t group_x = get_local_size(0);
	const size_t group_y = get_local_size(1);

	// globalni zaporedni index, upoštevamo globalne koordinate in širino
	const size_t g_idx = x + y * width;
	// lokalni zaporedni index, robne vrednosti dodamo +1
	const size_t l_idx = l_y * shared_size + l_x + 1;

	updatePixelPos(input, shared, x, y, width, height, l_x, l_y, shared_size, group_x, group_y, g_idx, l_idx);

	//Na podlagi lokalnega pomnilnika sedaj generiramo piksle
	Gx = -shared[l_idx - shared_size] - 2 * shared[l_idx - 1] -
		shared[l_idx + 1 * shared_size - 1] + shared[l_idx - 1 * shared_size + 1] +
		2 * shared[l_idx + 1] + shared[l_idx + 1 * shared_size + 1];

	Gy = -shared[l_idx - 1 * shared_size - 1] - 2 * shared[l_idx - shared_size] -
		shared[l_idx - 1 * shared_size + 1] + shared[l_idx + 1 * shared_size - 1] +
		2 * shared[l_idx + 1] + shared[l_idx + 1 * shared_size + 1];

	tempPixel = sqrt((float)(Gx * Gx + Gy * Gy));

	//Zapišemo v output
	if (tempPixel > 255)
		output[g_idx] = 255;
	else
		output[g_idx] = tempPixel;
}

static inline void poisciSive(int h, int w, __global int* slikaInput, __global int* siv) {
	int najmanjsa_vrednost_v_vrstici = INT_MAX;
	int indeks_najmanjse_vrednosti_v_vrstici;
	for (int i = 0; i < w; i++) {
		if (slikaInput[i] < najmanjsa_vrednost_v_vrstici) {
			najmanjsa_vrednost_v_vrstici = slikaInput[i];
			indeks_najmanjse_vrednosti_v_vrstici = i;
		}
	}
	//Shranimo prvi siv iz prve vrstice
	siv[0] = indeks_najmanjse_vrednosti_v_vrstici;
	int siv_c = 1;
	int leviElement;
	int desniElement;
	while (siv_c < h) {
		//Gledamo 3 primere, levi desni in srednji
		int indeksNaslednjeVrstice = return_global_id(indeks_najmanjse_vrednosti_v_vrstici, siv_c, w);
		int elementNaslednjeVrstice = slikaInput[indeksNaslednjeVrstice];

		if (indeks_najmanjse_vrednosti_v_vrstici - 1 >= 0) {
			leviElement = slikaInput[indeksNaslednjeVrstice -1];
		}

		if (indeks_najmanjse_vrednosti_v_vrstici + 1 < w) {
			desniElement = slikaInput[indeksNaslednjeVrstice + 1];
		}
		indeks_najmanjse_vrednosti_v_vrstici += leviElement < elementNaslednjeVrstice
			? (leviElement < desniElement ? -1 : 1) : (elementNaslednjeVrstice < desniElement ? 0 : 1);

		siv[siv_c] = indeks_najmanjse_vrednosti_v_vrstici;
		leviElement = desniElement = elementNaslednjeVrstice = INT_MAX;
		siv_c++;
	}

}

__kernel void odstraniSiv(
	__global int *inputSlika,
	__global int *outputSlika,
	__global int *siv,
	int width,
	int height) {
	int curX = get_global_id(0);
	int curY = get_global_id(1);
	
	int tmpX = curX;

	int koncniIndeks;
	if (curX < width && curY < height) {
		koncniIndeks = return_global_id(curX, curY, width);
		//v vsaki vrstici preskočim en piksel
		if (curX >= siv[curY]) {
			tmpX++;
		}
		outputSlika[koncniIndeks] = inputSlika[return_global_id(tmpX, curY, width + 1)];
	}
}
int return_global_id(int x, int y, int width) {
	return y * width + x;
}
