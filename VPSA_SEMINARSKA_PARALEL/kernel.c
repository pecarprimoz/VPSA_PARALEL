static inline int returnMinimumValue(int a, int b, int c);
static inline void poracunajVsote(int h, int w, __global int* slikaInput);
static inline int returnMinimumValue(int a, int b, int c) {
	if (a<b && a<c) {
		return a;
	}
	else if (b<a && b<c) {
		return b;
	}
	return c;
}

static inline void poracunajVsote(int h, int w, __global int* slikaInput) {
	//-2 zaradi tega, ker hoèemo zaèeti na zaèetku predzadnje vrstice, glej list za referenco
	int num_groups = w / 4;
	for (int i = h - 2; i >= 0; i--) {
		for(int j = 0; j < num_groups; j++){
		//gremo po celotni vrstici
			//trenutna pozicija glede na širino in stolpec v katerem smo plus trenutni indeks 
			int trenutna_pozicija_v_vektorju = (w*i) + j + (get_global_id(0)*num_groups);
			int trenutna_vrednost_v_vektorju = slikaInput[trenutna_pozicija_v_vektorju];
			//ROBNI PRIMER 1, CE SEM NA ZACETKU NE PREVERJAM LEVEGA
			if (j == 0) {
				//ÈE JE MOJ SPODNJI SOSED DIREKTNO POD MANO MANJŠI OD MOJEGA DESNEGA SOSEDA GA PRIŠTEJEM
				if (slikaInput[trenutna_pozicija_v_vektorju + w]<slikaInput[trenutna_pozicija_v_vektorju + (w + 1)]) {
					trenutna_vrednost_v_vektorju += slikaInput[trenutna_pozicija_v_vektorju + w];
				}
				else {
					trenutna_vrednost_v_vektorju += slikaInput[trenutna_pozicija_v_vektorju + (w + 1)];
				}
			}
			//ROBNI PRIMER 2, CE SEM NA KONCU NE PREVERJAM DESNEGA
			else if (j == w - 1) {
				if (slikaInput[trenutna_pozicija_v_vektorju + w]<slikaInput[trenutna_pozicija_v_vektorju + (w - 1)]) {
					trenutna_vrednost_v_vektorju += slikaInput[trenutna_pozicija_v_vektorju + w];
				}
				else {
					trenutna_vrednost_v_vektorju += slikaInput[trenutna_pozicija_v_vektorju + (w - 1)];
				}
			}
			//NI ROBNEGA PRIMERA, GLEDAM 3 SPODNJE SOSEDE
			else {
				int a = slikaInput[trenutna_pozicija_v_vektorju + (w - 1)];
				int b = slikaInput[trenutna_pozicija_v_vektorju + w];
				int c = slikaInput[trenutna_pozicija_v_vektorju + (w + 1)];
				trenutna_vrednost_v_vektorju += returnMinimumValue(a, b, c);
			}
			//Posodobimo trenutno sliko
			slikaInput[trenutna_pozicija_v_vektorju] = trenutna_vrednost_v_vektorju;
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
	}
}

void poisciSive(int h, int w, __global int* slikaInput, __global int* slikaInputOriginal) {

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
	//Zaènemo v prvi vrstici (ne v nièti, tam smo iskali prvotno vrednost)
	int trenutna_vrstica = 1;
	while (trenutna_vrstica<h) {
		//èe je trenutni indeks % w enak širini pomeni da smo na zaèetku, pregledamo le 2 elementa
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
		//èe je trenutni indeks % w enak širini -1 pomeni da smo na robu, pregledamo le 2 elementa
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


__kernel void seamCarving(__global int *slikaInputOriginal, int OriginalW, int OriginalH,
						  __global int *slikaKoncna, int KoncnaW, int KoncnaH,
						  __global int *slikaInput, int InputW, int InputH, int MAXITER)
{
	//Število iteracij je definirano v DEFINE
		//Višina in širina slike
		int cntW = 0;
		
		while (cntW < MAXITER) {
			int w = InputW;
		int h = InputH;
			poracunajVsote(h, w, slikaInput);
			poisciSive(h, w, slikaInput, slikaInputOriginal);

			KoncnaW = OriginalW - 1;
			KoncnaH = OriginalH;
			//Fun fact, èe sem imel for od 0 do velikosti slike, jo je izrisalo obratno, èe to delam obratno je pravilno
			int cnt = KoncnaW*KoncnaH;
			for (int i = InputW*InputH; i>0; i--) {
				if (slikaInputOriginal[i] != -1) {
					slikaKoncna[cnt] = slikaInputOriginal[i];
					cnt--;
				}
			}
			//Posodobim slikaInputOriginal, ki mi hrani vrednosti za rezanje
			OriginalH = KoncnaH;
			OriginalW = KoncnaW;
			for (int i = 0; i<KoncnaW*KoncnaH; i++) {
				slikaInputOriginal[i] = slikaKoncna[i];
			}

			//memcpy(slikaInputOriginal, slikaKoncna, KoncnaW*KoncnaH * sizeof(int));

			//Posodobim slikaInput, ki mi hrani vsote
			InputH = KoncnaH;
			InputW = KoncnaW;
			for (int i = 0; i<KoncnaW*KoncnaH; i++) {
				slikaInput[i] = slikaKoncna[i];
			}

			cntW++;
		}
		//slikaKoncna[0]=slikaInput[0];
		
		
		//Testiranje ene iteracije izvajanje postopka
		
		//Sedaj imamo v slikaInput poraèunane vse vsote, gremo gledati minimum

		
		/*
		//Sedaj imamo postavljene vrednosti na -1, tam kjer smo imeli najmanše vsote, te bomo izrezali

		//Width-1, ker režemo po širini
		
		//memcpy(slikaInput, slikaKoncna, KoncnaW*KoncnaH * sizeof(int));
		//Poveèam št. iteracij
		*/
		

}