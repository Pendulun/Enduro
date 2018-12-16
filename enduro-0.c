#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

#define NUMCARS 6
#define INC 4.0
#define INCACELERA 2.0
#define INCFREIA -0.5
#define INC2 1.0
#define INCLADO 0.65
#define TAXA 60

const float FPS = 100;  
const int SCREEN_W = 960;
const int SCREEN_H = 540;

int SKY_H;

float ymin; 					// Distância y max do carro até o  começo da pista
float TRACK_LEFT_X_MAX;			//máximo valor de x do triângulo esquerdo

float TRACK_TOP_W;
float TRACK_BOTTOM_W;

float TRACK_TOP_LEFT_X;
float TRACK_TOP_LEFT_Y;
float TRACK_BOTTOM_LEFT_X;
float TRACK_BOTTOM_LEFT_Y;

float TRACK_TOP_RIGHT_X;
float TRACK_TOP_RIGHT_Y;
float TRACK_BOTTOM_RIGHT_X;
float TRACK_BOTTOM_RIGHT_Y;

float THETA,TAN_THETA;

ALLEGRO_FONT *size_32;

int score = 0;

typedef struct Carro {
	float car_w_total,car_h_total;
	float car_w,car_h,car_t_l_x,car_t_l_y,car_b_r_x,car_b_r_y; 				//posicoes da carroceria
	float wheel_w,wheel_h; 													//tamanho das rodas
	float wheel_l_t_y,wheel_l_t_x,wheel_l_b_y,wheel_l_b_x; 					//roda esquerda 
	float wheel_r_t_y,wheel_r_t_x,wheel_r_b_y,wheel_r_b_x; 					//roda direita
	float xoffset; 															//distancia do x para a margem esquerda da pista
	float y2, x2;															//altura e comprimento relativos do triangulo gerado
	float incLado;
	ALLEGRO_COLOR cor;
} Carro;

Carro jogador;  			// carro do jogador
Carro oponentes[NUMCARS]; 		//carros oponentes

// confere se os carros bateram
int bounding_box_collision(int b1_x, int b1_y, int b1_w, int b1_h, int b2_x, int b2_y, int b2_w, int b2_h){
    if ((b1_x > b2_x + b2_w - 1) || // is b1 on the right side of b2?
        (b1_y > b2_y + b2_h - 1) || // is b1 under b2?
        (b2_x > b1_x + b1_w - 1) || // is b2 on the right side of b1?
        (b2_y > b1_y + b1_h - 1))   // is b2 under b1?
    {
        // no collision
        return 0;
    }

    // collision
    return 1;
}


// tela game over
int gameOver(int score,ALLEGRO_DISPLAY *display,ALLEGRO_FONT *size,ALLEGRO_EVENT_QUEUE *event_queue){
	//Tela de fundo branca
	ALLEGRO_COLOR BKG_COLOR = al_map_rgb(255,255,255); 
	al_set_target_bitmap(al_get_backbuffer(display));
	al_clear_to_color(BKG_COLOR);
	//variavel do tipo char[] que recebe um texto
	char my_text[20];

	al_draw_text(size,al_map_rgb(0, 0, 0), SCREEN_W/2, SCREEN_H/2 -66, 0, "GAME OVER!");

   	sprintf(my_text, "Pontos: %d", score);

   //imprime o texto armazenado em my_text na posicao x=SCREEN_W/2,y=SCREEN_H/2 e com a cor rgb(0,0,0)
    al_draw_text(size, al_map_rgb(0, 0, 0), SCREEN_W/2, SCREEN_H/2, 0, my_text);
    //abre arquivo com o recorde
    FILE *arq;
    int recorde;
	arq = fopen("recorde.txt", "r");
	fscanf(arq, "%d", &recorde);
	fclose(arq);
	//se a pontuação for maior do que o recorde
	if(score>recorde){
    	al_draw_text(size, al_map_rgb(0, 0, 0), SCREEN_W/2, SCREEN_H/2 - 33, 0, "NOVO RECORDE!");
    	arq = fopen("recorde.txt", "w");
    	//atualiza o recorde
    	fprintf(arq,"%d",score);
    	//fecha o arquivo
		fclose(arq);
	}
	al_flip_display();
	ALLEGRO_EVENT ev2;
	while(1){
		//espera por um evento e o armazena na variavel de evento ev
      al_wait_for_event(event_queue, &ev2);

		if(ev2.type == ALLEGRO_EVENT_KEY_DOWN) {
			//verifica qual tecla foi
			switch(ev2.keyboard.keycode) {
				//se a tecla for o enter
				case ALLEGRO_KEY_ENTER:
					return 1;
					break;
				case ALLEGRO_KEY_ESCAPE:
					return 0;
					break;
			}
		}else if(ev2.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			return 0;
		}
	}
}


//calcula os novos valores do carro de acordo com sua proporção 
void calcDimCarro(Carro *carro){ 						
	carro->xoffset = carro->wheel_l_t_x-(TRACK_LEFT_X_MAX - carro->x2); 	// distância até a linha lateral esquerda da pista
	carro->wheel_h = carro->car_h_total; 									// altura total do carro = altura total da roda 
	carro->car_h =carro->wheel_h / 1.5; 									// altura total da carroceria
	carro->wheel_w =carro->car_w_total/4;									// comprimeto total da roda
	carro->car_w = carro->car_w_total/2; 									// comprimento da carroceria

	//SOBRE OS PNEUS
	
	carro->wheel_l_b_x = carro->wheel_l_t_x + carro->wheel_w;						//bot_left_x
	carro->wheel_l_b_y = carro->wheel_l_t_y+carro->wheel_h;							//bot_left_y
	carro->wheel_r_t_x = carro->wheel_l_t_x + carro->wheel_w+carro->car_w;			//top_rigth_x
	carro->wheel_r_t_y = carro->wheel_l_t_y;										//top_right_y
	carro->wheel_r_b_x = carro->wheel_l_t_x+(2*carro->wheel_w)+carro->car_w; 		//bot_right_x
	carro->wheel_r_b_y = carro->wheel_l_t_y+carro->wheel_h;							//bot_right_y


	//SOBRE A CARROCERIA

	carro->car_t_l_y = carro->wheel_l_t_y + ((carro->wheel_h - carro->car_h))/2;  	//top_left_y
	carro->car_t_l_x = carro->wheel_l_t_x + carro->wheel_w;							//top_left_x
	carro->car_b_r_y = carro->car_t_l_y + carro->car_h;								//bot_right_y
	carro->car_b_r_x = carro->car_t_l_x + carro->car_w;								//bot_left_x


}

float calcX2(float y2){
	return y2/TAN_THETA;
}

float calcY2(float top_y){
	return top_y - ymin;
}


//Calcula o comprimento e a altura máxima do carro naquele momento
void calcTamMaxCar(Carro *carro){
	// comprimento máximo do carro no momento
	carro->car_w_total = carro->x2/6;
	// altura máxima do carro no momento
	carro->car_h_total = carro->y2/10;
}

//incrementa valores nas posições do topo esquerdo da roda esquerda
void attPosCarro(Carro *carro,float incX, float incY){

carro->wheel_l_t_y += incY;
carro->wheel_l_t_x += incX;
carro->y2 = calcY2(carro->wheel_l_t_y);			//o novo y2 relativo
carro->x2 = calcX2(carro->y2);					//o novo x2 relativo
calcTamMaxCar(carro);    						//calcula os novos tamanhos totais do carro
calcDimCarro(carro);							//define as novas posições do carro
}

//define as posições iniciais do carro
void PosCarro(Carro *carro,int i){
	if(i==1){													//se for o carro do jogador
		carro->wheel_l_t_y = SCREEN_H - 60;    					//top_left_y pneu
		carro->wheel_l_t_x = (SCREEN_W/2) - (TRACK_TOP_W/2);	//top_left_x pneu
		//cor do carro
		ALLEGRO_COLOR CAR_COLOR = al_map_rgb(255,99,71);
		carro->cor = CAR_COLOR;

	}else if(i==2){		

		//se for um carro oponente
		carro->wheel_l_t_y = SKY_H+1;
		int pos =  (int)rand()%5;

		switch(pos){
			case 0:
				carro->wheel_l_t_x = TRACK_TOP_LEFT_X ;
				carro->incLado = -1.0;
			break;

			case 1:
				carro->wheel_l_t_x = TRACK_TOP_LEFT_X + TRACK_TOP_W/5;
				carro->incLado = -INCLADO;
			break;

			case 2:
				carro->wheel_l_t_x = TRACK_TOP_LEFT_X + 2*TRACK_TOP_W/5;
				carro->incLado = -0.2;
				break;

			case 3:
				carro->wheel_l_t_x = TRACK_TOP_LEFT_X + 3*TRACK_TOP_W/5;
				carro->incLado = 0.3;
				break;

			case 4:
				carro->wheel_l_t_x = TRACK_TOP_LEFT_X + 4*TRACK_TOP_W/5;
				carro->incLado = INCLADO;
				break;
			default:
				carro->wheel_l_t_x = TRACK_TOP_LEFT_X + 3*TRACK_TOP_W/5;
				carro->incLado = INCLADO;
				break;
		}
		//cor aleatória para o carro
		carro->cor = al_map_rgb((int)rand()%255 ,(int)rand()%255 ,(int)rand%255);
	}

	carro->y2 = calcY2(carro->wheel_l_t_y);					//calcula o y2 do triangulo
	carro->x2 = calcX2(carro->y2);							//calcula o x2 do triangulo
	calcTamMaxCar(carro);
	calcDimCarro(carro);
}

//inicia variáveis globais
void init_global_vars() {
	
	SKY_H = SCREEN_H/4;

	TRACK_TOP_W = SCREEN_W/100;
	TRACK_BOTTOM_W = SCREEN_W/1.2;
	
	TRACK_TOP_LEFT_X = SCREEN_W/2 - TRACK_TOP_W/2;
	TRACK_TOP_LEFT_Y = SKY_H;
	TRACK_BOTTOM_LEFT_X = SCREEN_W/2 - TRACK_BOTTOM_W/2;
	TRACK_BOTTOM_LEFT_Y = SCREEN_H;
	
	TRACK_TOP_RIGHT_X = SCREEN_W/2 + TRACK_TOP_W/2;
	TRACK_TOP_RIGHT_Y = SKY_H;
	TRACK_BOTTOM_RIGHT_X = SCREEN_W/2 + TRACK_BOTTOM_W/2;
	TRACK_BOTTOM_RIGHT_Y = SCREEN_H;

	TRACK_LEFT_X_MAX = ((TRACK_BOTTOM_W - TRACK_TOP_W)/2)+((SCREEN_W-TRACK_BOTTOM_W)/2);
	ymin = SKY_H;

//track angle
	TAN_THETA = (SCREEN_H-SKY_H)/(TRACK_BOTTOM_W);
	THETA = atan(TAN_THETA) * (180.0/3.14159265);
	
}

//desenha o carro
void draw_car(Carro *carro){
	//carroceria
	al_draw_filled_rectangle(carro->car_t_l_x,carro->car_t_l_y,carro->car_b_r_x,carro->car_b_r_y,carro->cor);

	//rodas
	//esquerda
	al_draw_filled_rectangle(carro->wheel_l_t_x,carro->wheel_l_t_y,carro->wheel_l_b_x,carro->wheel_l_b_y,al_map_rgb(105,105,105));
	//direita
	al_draw_filled_rectangle(carro->wheel_r_t_x,carro->wheel_r_t_y,carro->wheel_r_b_x,carro->wheel_r_b_y, al_map_rgb(105,105,105));
}

void draw_inner_mountain(float x1, float y1, float x2, float y2,float x3, float y3){
	al_draw_filled_triangle(x1,y1,x2,y2,x3,y3,al_map_rgb(61,16,1));
}

void draw_snow_mountain(float x1, float y1, float x2, float y2,float x3, float y3){
	al_draw_filled_triangle(x1,y1,x2,y2,x3,y3,al_map_rgb(255,255,255));
}

void draw_mountains(float x1, float y1, float x2, float y2,float x3, float y3){
	//desenha as bases
	al_draw_filled_triangle(x1,y1,x2,y2,x3,y3,al_map_rgb(99,45,1));
	al_draw_filled_triangle(x1+20,y1*1.5,x2+20,y2,x3+20,y3,al_map_rgb(99,45,1));
	al_draw_filled_triangle(x1-20,y1*1.5,x2-20,y2,x3-20,y3,al_map_rgb(99,45,1));
	//desenha as partes de dentro
	draw_inner_mountain(x1,y1*1.6,x2,y2,x3,y3);
	draw_inner_mountain(x1+20,(y1*1.5)*1.6,x2+20,y2,x3+20,y3);
	draw_inner_mountain(x1-20,(y1*1.5)*1.6,x2-20,y2,x3-20,y3);

	//desenha os topos com neve
	draw_snow_mountain(x1,y1,x1-8, y1+15,x1+8,y1+15);
	draw_snow_mountain(x1+20,y1*1.5,x1+20-7, (y1*1.5)+11,x1+20+7,(y1*1.5)+11);
	draw_snow_mountain(x1-20,y1*1.5,x1-20-7, (y1*1.5)+11,x1-20+7,(y1*1.5)+11);

}

void draw_scenario(ALLEGRO_DISPLAY *display,ALLEGRO_FONT *size) {


	//grass
	ALLEGRO_COLOR BKG_COLOR = al_map_rgb(55,171,38); 
	al_set_target_bitmap(al_get_backbuffer(display));
	al_clear_to_color(BKG_COLOR);   
	
	//sky
	al_draw_filled_rectangle(0, 0, SCREEN_W, SKY_H, al_map_rgb(0,255,247));
   
   //desenha a pista:
   al_draw_line(TRACK_TOP_LEFT_X, TRACK_TOP_LEFT_Y, TRACK_BOTTOM_LEFT_X, TRACK_BOTTOM_LEFT_Y, al_map_rgb(255,255,255), 10); 
   al_draw_line(TRACK_TOP_RIGHT_X, TRACK_TOP_RIGHT_Y, TRACK_BOTTOM_RIGHT_X, TRACK_BOTTOM_RIGHT_Y, al_map_rgb(255,255,255), 10);  

   //desenha as montanhas
   draw_mountains(SCREEN_W/2,SKY_H/4,SCREEN_W/2 - SKY_H*3/8,SKY_H,SCREEN_W/2 + SKY_H*3/8,SKY_H);
   draw_mountains(SCREEN_W/2 + 300,SKY_H/4,SCREEN_W/2 - SKY_H*3/8 +300,SKY_H,SCREEN_W/2 + SKY_H*3/8 + 300,SKY_H);
   draw_mountains(SCREEN_W/2 - 300,SKY_H/4,SCREEN_W/2 - SKY_H*3/8 -300,SKY_H,SCREEN_W/2 + SKY_H*3/8 - 300,SKY_H);
   
  //DESENHA O CARRO
   draw_car(&jogador);
   //variavel do tipo char[] que recebe um texto
	char my_text[20];

   	sprintf(my_text, "Pontos: %d", score);

   //imprime o texto armazenado em my_text na posicao x=10,y=10 e com a cor rgb(0,0,0)
    al_draw_text(size, al_map_rgb(0, 0, 0), 10, 10, 0, my_text);

}

int main(int argc, char **argv){
	
	srand((unsigned)time(NULL));
	ALLEGRO_DISPLAY *display = NULL;
	ALLEGRO_EVENT_QUEUE *event_queue = NULL;
	ALLEGRO_TIMER *timer = NULL;
	ALLEGRO_AUDIO_STREAM *musica = NULL;
	
	init_global_vars();
	PosCarro(&jogador,1);
   
	//----------------------- rotinas de inicializacao ---------------------------------------
   if(!al_init()) {
      fprintf(stderr, "failed to initialize allegro!\n");
      return -1;
   }
   
    if(!al_init_primitives_addon()){
		fprintf(stderr, "failed to initialize primitives!\n");
        return -1;
    }
 
   timer = al_create_timer(1.0 / FPS);
   if(!timer) {
      fprintf(stderr, "failed to create timer!\n");
      return -1;
   }
 
   display = al_create_display(SCREEN_W, SCREEN_H);
   if(!display) {
      fprintf(stderr, "failed to create display!\n");
      al_destroy_timer(timer);
      return -1;
   }

	//inicializa o modulo allegro que carrega as fontes
    al_init_font_addon();

    
	//inicializa o modulo allegro que entende arquivos tff de fontes
    if (!al_init_ttf_addon()){
        fprintf(stderr, "Failed to initialize add-on allegro_ttf\n");
        return -1;
    }


    //inicia funções relativas ao áudio
    if (!al_install_audio())
    {        fprintf(stderr, "Falha ao inicializar áudio.\n");
        return false;
    }
    //inicializa os codecs necessários para carregar os diversos formatos de arquivo suportados
   if (!al_init_acodec_addon())
    {
        fprintf(stderr, "Falha ao inicializar codecs de áudio.\n");
        return false;
    }
    //recebe a quantidade de canais que devem ser alocados ao mixer principal
    if (!al_reserve_samples(1)){
        fprintf(stderr, "Falha ao alocar canais de áudio.\n");
        return false;
    }
    //In the hall of the mountain king - edvard krieg
    musica = al_load_audio_stream("Line Rider - Mountain King.ogg", 4, 1024);
    if (!musica){
        fprintf(stderr, "Falha ao carregar audio.\n");
        al_destroy_display(display);
        return false;
    }
    al_attach_audio_stream_to_mixer(musica, al_get_default_mixer());
    al_set_audio_stream_playmode(musica,
   ALLEGRO_PLAYMODE_LOOP);
    al_set_audio_stream_playing(musica, true);


	//carrega o arquivo arial.ttf da fonte Arial e define que sera usado o tamanho 32 (segundo parametro)
    size_32 = al_load_font("arial.ttf", 32, 1);   	


   event_queue = al_create_event_queue();
   if(!event_queue) {
      fprintf(stderr, "failed to create event_queue!\n");
      al_destroy_display(display);
      al_destroy_timer(timer);
      return -1;
   }
   
   al_install_keyboard();
   
	
	//registra na fila de eventos que eu quero identificar quando a tela foi alterada
   al_register_event_source(event_queue, al_get_display_event_source(display));
	//registra na fila de eventos que eu quero identificar quando o tempo alterou de t para t+1
   al_register_event_source(event_queue, al_get_timer_event_source(timer));
   
   al_register_event_source(event_queue, al_get_keyboard_event_source());   
    
  

	//reinicializa a tela
   al_flip_display();
	//inicia o temporizador
   al_start_timer(timer);



   
   int playing = 1,opCars=0,carsUtili=0;
   float count = 0.0;
   ALLEGRO_EVENT ev;
   bool right=false,left=false,up=false,down=false,pause=false;
	//enquanto playing for verdadeiro, faca:
   while(playing) {

	  //espera por um evento e o armazena na variavel de evento ev
      al_wait_for_event(event_queue, &ev);
	  if(pause){
	  	if(ev.type== ALLEGRO_EVENT_KEY_UP && ev.keyboard.keycode == ALLEGRO_KEY_P){
	  		pause=false;
	  	}else{
	  		continue;
	  	}
	  }

	if(ev.type == ALLEGRO_EVENT_KEY_DOWN) {
		//verifica qual tecla foi
		switch(ev.keyboard.keycode) {
			//se a tecla for o W
			case ALLEGRO_KEY_UP:

			case ALLEGRO_KEY_W:
				up=true;

			break;
			case ALLEGRO_KEY_DOWN:
			//se a tecla for o S
			case ALLEGRO_KEY_S:
				down = true;
			break;

			case ALLEGRO_KEY_LEFT:				//Seta para esquerda

			case ALLEGRO_KEY_A:					// Letra A
				if((jogador.wheel_l_t_x - INC )>=((SCREEN_H-SKY_H)*TRACK_BOTTOM_LEFT_X)/(jogador.y2)){
					attPosCarro(&jogador,-INC ,0.0);
					left=true;
				}
			break;

			case ALLEGRO_KEY_RIGHT:				//Seta para direita

			case ALLEGRO_KEY_D:					// Letra D
				if((jogador.wheel_l_t_x+2*(jogador.wheel_w)+(jogador.car_w) + INC)<=(TRACK_BOTTOM_RIGHT_X - 12.571)){
					attPosCarro(&jogador,INC,0.0);
					right=true;
				}
			break;

			case ALLEGRO_KEY_ESCAPE:
				playing = 0;
			break;

			case ALLEGRO_KEY_P:
				pause=true;
			}

		}	 
	else if(ev.type == ALLEGRO_EVENT_KEY_UP) {
		//verifica qual tecla foi
		switch(ev.keyboard.keycode) {
			//para cima
			case ALLEGRO_KEY_UP:

			case ALLEGRO_KEY_W:
			up=false;
			break;
			//para baixo
			case ALLEGRO_KEY_DOWN:
			case ALLEGRO_KEY_S:
				down=false;
			break;

			case ALLEGRO_KEY_LEFT:	

			case ALLEGRO_KEY_A:
				left=false;
			break;

			case ALLEGRO_KEY_RIGHT:	

			case ALLEGRO_KEY_D:
				right=false;
			break;
			}

		}
	
	//se o tipo de evento for um evento do temporizador, ou seja, se o tempo passou de t para t+1
    else if(ev.type == ALLEGRO_EVENT_TIMER) {
    	count++;
    	if(count>=TAXA){
    			if(carsUtili < NUMCARS){
    				PosCarro(&oponentes[carsUtili],2);
    				opCars++;
    				carsUtili++;
    			}
    	}
		
		//Se estiver apertando esquerda mas não direita ao msm tempo
		if(left && !right){
			//confere se não vai passar da linha da esquerda
			if((jogador.wheel_l_t_x - INC)>=((SCREEN_H-SKY_H)*TRACK_BOTTOM_LEFT_X)/(jogador.y2)){
				attPosCarro(&jogador,-INC,0.0);
			}
		}

		//Se estiver apertando direita mas não esquerda ao mesmo tempo
		if(right && !left){
			//confere se não vai passar da linha direita
			if((jogador.wheel_l_t_x+2*(jogador.wheel_w)+(jogador.car_w) + INC)<=(TRACK_BOTTOM_RIGHT_X - 12.571)){
				attPosCarro(&jogador,INC,0.0);
			}
   		}
   		draw_scenario(display,size_32);
   		//desenha os oponentes dps do cenário
   		if(opCars>0){
    		for(int i=0;i<opCars;i++){
    			draw_car(&oponentes[i]);
    			//Att a posicao de cada carro oponente já existente
    			if(up){
    				attPosCarro(&oponentes[i],(oponentes[i].incLado*(INC2+INCACELERA))/(INC2),INC2 + INCACELERA);
    				count+=3;
    			}else if(down){
    				attPosCarro(&oponentes[i],(oponentes[i].incLado*(INC2+INCFREIA))/(INC2),INC2 + INCFREIA);
    				count+=0.5;
    			}else{
    				attPosCarro(&oponentes[i],oponentes[i].incLado,INC2);
    			}

    			//confere se bateu
    			if(bounding_box_collision(oponentes[i].wheel_l_t_x, oponentes[i].wheel_l_t_y,oponentes[i].car_w_total,oponentes[i].car_h_total,
    				jogador.wheel_l_t_x,jogador.wheel_l_t_y,jogador.car_w_total,jogador.car_h_total)){
    					//mostra tela de game over e continua ou não jogando
    					if(gameOver(score,display,size_32,event_queue)){
    						draw_scenario(display,size_32);
    					}else{
    						playing=0;
    					}
    					carsUtili=0;
    					opCars=0;
    					score=0;
    					count=0.0;
    					right=false;
    					left=false;
    					up=false;
    					down=false;
    					// Coloca o carro do jogador na posição inicial
    					PosCarro(&jogador,1);
    			}

    			//se ele passou do tamanho da tela, coloca ele lá em cima de novo
    			if(oponentes[i].wheel_l_t_y>SCREEN_H && count >= TAXA){
    				PosCarro(&oponentes[i],2);
    				score++;
    			}
    		}
    	}

    	if(count>=TAXA){
    		count=0.0;
    	}
		//reinicializo a tela
		al_flip_display();
}
	//se o tipo de evento for o fechamento da tela (clique no x da janela)
	else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
		playing = 0;
	}
  } //fim do while
     
     //termina a música
 	al_set_audio_stream_playing(musica, false);
	
	//reinicializa a tela
	al_flip_display();	
    al_rest(1);	
	
  	al_destroy_audio_stream(musica);
   	al_destroy_timer(timer);
   	al_destroy_display(display);
   	al_destroy_event_queue(event_queue);
   
 
   return 0;
}