#define NUM_THREADS 4
static inline int returnMinimumValue(int a, int b, int c);
static inline int returnMinimumValue(int a, int b, int c) {
	if (a<b && a<c) {
		return a;
	}
	else if (b<a && b<c) {
		return b;
	}
	return c;
}
__kernel void seamCarving(__global int *slikaInput, int InputW, int InputH)
{
	int vector_length_for_thread;
	int w;
	int h;
	w = InputW;
	h = InputH;
	vector_length_for_thread = w / NUM_THREADS;
	int left;
	int middle;
	int right;

	for (int i = h - 2; i >= 0; i--) {
		for (int j = 0; j < vector_length_for_thread; j++) {
			//gremo po celotni vrstici
			//trenutna pozicija glede na �irino in stolpec v katerem smo plus trenutni indeks 
			int trenutna_pozicija_v_vektorju = (w / NUM_THREADS*i) + j + (get_global_id(0)*vector_length_for_thread);
			int trenutna_vrednost_v_vektorju = slikaInput[trenutna_pozicija_v_vektorju];
			left = slikaInput[trenutna_pozicija_v_vektorju + (w - 1)];
			middle = slikaInput[trenutna_pozicija_v_vektorju + (w)];
			right = slikaInput[trenutna_pozicija_v_vektorju + (w + 1)];
			//ROBNI PRIMER 1, CE SEM NA ZACETKU NE PREVERJAM LEVEGA
			if (j == 0) {
				//�E JE MOJ SPODNJI SOSED DIREKTNO POD MANO MANJ�I OD MOJEGA DESNEGA SOSEDA GA PRI�TEJEM
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
}