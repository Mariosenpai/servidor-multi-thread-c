#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gmp.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

// how many decimal digits the algorithm generates per iteration:
#define DIGITS_PER_ITERATION 14.1816474627254776555

#define NUMERO_THREAD 2
#define TEMPOREQ 100

struct Argumentos{
	int id;
	int quantidadeCasasPi;
	int tempoEspera;
};

pthread_mutex_t mutex;
int thread_ocupadas[NUMERO_THREAD] = {0};//0 livre, 1 ocupada
int quantidade_requisicoes = 0;
char* nomePasta = "requisicoes";


char *chudnovsky(unsigned long digits)
{
	mpf_t result, con, A, B, F, sum;
	mpz_t a, b, c, d, e;
	char *output;
	mp_exp_t exp;
	double bits_per_digit;

	unsigned long int k, threek;
	unsigned long iterations = (digits/DIGITS_PER_ITERATION)+1;
	unsigned long precision_bits;

	// roughly compute how many bits of precision we need for
	// this many digit:
	bits_per_digit = 3.32192809488736234789; // log2(10)
	precision_bits = (digits * bits_per_digit) + 1;

	mpf_set_default_prec(precision_bits);

	// allocate GMP variables
	mpf_inits(result, con, A, B, F, sum, NULL);
	mpz_inits(a, b, c, d, e, NULL);

	mpf_set_ui(sum, 0); // sum already zero at this point, so just FYI

	// first the constant sqrt part
	mpf_sqrt_ui(con, 10005);
	mpf_mul_ui(con, con, 426880);

	// now the fun bit
	for (k = 0; k < iterations; k++) {
		threek = 3*k;

		mpz_fac_ui(a, 6*k);  // (6k)!

		mpz_set_ui(b, 545140134); // 13591409 + 545140134k
		mpz_mul_ui(b, b, k);
		mpz_add_ui(b, b, 13591409);

		mpz_fac_ui(c, threek);  // (3k)!

		mpz_fac_ui(d, k);  // (k!)^3
		mpz_pow_ui(d, d, 3);

		mpz_ui_pow_ui(e, 640320, threek); // -640320^(3k)
		if ((threek&1) == 1) { mpz_neg(e, e); }

		// numerator (in A)
		mpz_mul(a, a, b);
		mpf_set_z(A, a);

		// denominator (in B)
		mpz_mul(c, c, d);
		mpz_mul(c, c, e);
		mpf_set_z(B, c);

		// result
		mpf_div(F, A, B);

		// add on to sum
		mpf_add(sum, sum, F);
	}

	// final calculations (solve for pi)
	mpf_ui_div(sum, 1, sum); // invert result
	mpf_mul(sum, sum, con); // multiply by constant sqrt part

	// get result base-10 in a string:
	output = mpf_get_str(NULL, &exp, 10, digits, sum); // calls malloc()

	// free GMP variables
	mpf_clears(result, con, A, B, F, sum, NULL);
	mpz_clears(a, b, c, d, e, NULL);

	return output;
}

void* thread_trabalhadoras(void* arg){
	
	struct Argumentos *argumento = (struct Argumentos*)arg;


	//entra na zona critica
	pthread_mutex_lock(&mutex);
	//printf("Entrou na zona critica\n");

	//torno ela ocupada
	int id = argumento->id;
	thread_ocupadas[id] = 1;
	char* pi;
	int tempoEspera = argumento->tempoEspera;
	int quantidade_digitos_pi = argumento->quantidadeCasasPi;
	quantidade_requisicoes++;

	//printf("id=%d, temp=%d, qnt_req=%d\n", id,tempoEspera,quantidade_requisicoes);

	
	//gerar o numero pi
	pi = chudnovsky(quantidade_digitos_pi);

	//transferer para outra ja formatada
	char piCompleto[200];
	sprintf(piCompleto,"%.1s.%s", pi, pi+1);
	

	//verifica se a pasta existe se não ele criar ela
	DIR *diretorio = opendir(nomePasta);

	if(diretorio){
		closedir(diretorio);
	}else{
		if(mkdir(nomePasta, 0777) != 0){
			printf("Falha ao criar a pasta");
			return NULL;
		}
	}

	//tipo da externção
	char* tipo = ".txt";

	//transforma o id em uma string
	char id_string[20];
	sprintf(id_string,"%s/%d", nomePasta, id);
	
	char* nome_arquivo_requisicao = strcat(id_string,tipo);


	//criar arquivo
	FILE *arquivo = fopen(nome_arquivo_requisicao,"a");


	if(arquivo == NULL){
		printf("Erro ao criar o arquivo.\n");
		return NULL;
	}
	
	//tempo espera ate escrever no arquivo
	usleep(tempoEspera*1000);

	//escrever no arquivo
	fprintf(arquivo, "%d;%d;%d;%s\n",quantidade_requisicoes,quantidade_digitos_pi,tempoEspera,piCompleto);

	free(pi);
	fclose(arquivo);

	//deixa a thread livre
	thread_ocupadas[id] = 0;
	//sair da zone critica
	free(argumento);
	pthread_mutex_unlock(&mutex);

	//sched_yield();

	return NULL;
	
}

int main(int argc, char **argv)
{

	pthread_t t[NUMERO_THREAD];
	
	int quantidade_requisicoes;

	char* nome_arquivo = "requisicoes.txt";

	//pi
	int quantidade_digitos_min = 10;
	int quantidade_digitos_max = 100;
	int quantidade_digitos_pi;

	//milesegundos
	int quantidade_tempo_min = 150;
	int quantidade_tempo_max = 500;
	int quantidade_tempo;

	printf("Quantidade de requisicoes desejadas:");
	scanf("%d", &quantidade_requisicoes);

	//criar arquivo
	FILE *arquivo = fopen(nome_arquivo,"w");

	if(arquivo == NULL){
		printf("Erro ao criar o arquivo.\n");
		return 1;
	}
	//criar arquivo

	//escolher os valores aleatorios
	printf("Criando arquivo de requisicoe...\n");
	srand(time(NULL)); //semente
	int i;
	for(i =0;i<quantidade_requisicoes; i++){
		quantidade_digitos_pi = rand() %(quantidade_digitos_max - quantidade_digitos_min + 1) + quantidade_digitos_min;
		quantidade_tempo = rand() %(quantidade_tempo_max - quantidade_tempo_min + 1) + quantidade_tempo_min;
	
		//escrever no arquivo
		fprintf(arquivo, "%d;%d\n",quantidade_digitos_pi,quantidade_tempo);
	}
	fclose(arquivo);

	//Ler os arquivos

	printf("Mostrar o arquivo requisicoes\n");
	FILE *mostrar_requisicoes = fopen(nome_arquivo, "r");

	if(arquivo == NULL){
		printf("Erro ao abrir o arquivo");
		return 1;
	}

	
	int num_pi, tempo;
	int id = 0;
	int y =0;
	while (fscanf(mostrar_requisicoes, "%d;%d", &num_pi, &tempo) == 2){
		

		printf("id=%d, temp=%d, pi=,%d\n",y, tempo,num_pi);
		y++;

	}
	fclose(mostrar_requisicoes);

	printf("\n\n");



	printf("\nCriando arquivos com o log das thread...\n");
	FILE *arquivo_leituraa = fopen(nome_arquivo, "r");

	if(arquivo == NULL){
		printf("Erro ao abrir o arquivo");
		return 1;
	}

	//thread dispacher
	while (fscanf(arquivo_leituraa, "%d;%d", &num_pi, &tempo) == 2){
		//printf("pi %d tempo %d\n", num_pi,tempo);

		//criar thread
		struct Argumentos* argumento = (struct Argumentos*)malloc(sizeof(struct Argumentos));

		int id_thread = id % NUMERO_THREAD;
		argumento->tempoEspera = tempo;
		argumento->quantidadeCasasPi = num_pi;
		argumento->id = id_thread;
		
		//verifica se a thread esta ocupada
		if(thread_ocupadas[id_thread] == 0){
			if(pthread_create(&t[id_thread], NULL, thread_trabalhadoras, argumento) != 0){
				printf("Falha ao criar a thread\n");
				return 1;
			}
			printf("Inicialização da thread %d \n", id_thread);
			
		}else{
			printf("Encontrou uma thread ocupada\n");
			//caso a thread esteja ocupada
			//ele irar incrementar a lista circula das thread ate encontrar uma thread livre
			int prox =0;
			while(1){
				int id_thread_prox = (id_thread+prox) % NUMERO_THREAD;
				//novo valor de id passado no argumento
				argumento->id = id_thread_prox;
				//verificas se a proxima thread estar livre
				if(thread_ocupadas[id_thread_prox]==0){
					pthread_create(&t[id_thread_prox], NULL, thread_trabalhadoras, argumento);
					printf("Inicialização da proxima thread %d \n", id_thread_prox);
					break;
				}
				prox++;
			}
			

		}

		
		id++;
		//espera um tempo
		usleep(TEMPOREQ * 1000);

	}

	printf("\nTerminando as thread restantes...\n");
	int j;
	int termina_thread;
	
	//determina o tamanho do loop do join onde espera o terminio das thread
	if(NUMERO_THREAD > quantidade_requisicoes){
		termina_thread = quantidade_requisicoes;
	}else{
		termina_thread = NUMERO_THREAD;
	}

	for(j =0; j < termina_thread;j++){

		pthread_join(t[j],NULL);

		printf("Finalização da thread %d \n", j);
		
	}


	fclose(arquivo_leituraa);


	return 0;
}
