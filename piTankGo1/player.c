
#include "player.h"
#define GPIO_PIN 23 // El tone que viene en el enunciado

//------------------------------------------------------
// PROCEDIMIENTOS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------

static void timer_player_duracion_nota_actual_isr (union sigval value) {

	piLock(PLAYER_FLAGS_KEY);
	flags_player |= FLAG_NOTA_TIMEOUT;
	piUnlock(PLAYER_FLAGS_KEY);
}

//------------------------------------------------------
// PROCEDIMIENTOS DE INICIALIZACION DE LOS OBJETOS ESPECIFICOS
//------------------------------------------------------

int InicializaEfecto (TipoEfecto *p_efecto, char *nombre, int *array_frecuencias, int *array_duraciones, int num_notas) {

	strcpy(p_efecto->nombre, nombre);

	int j,k; // Si no, el bucle falla

	for (j = 0; j < num_notas; j++) {
		p_efecto->frecuencias[j] = array_frecuencias[j];
	}

	for (k = 0; k < num_notas; k++) {
			p_efecto->duraciones[k] = array_duraciones[k];
	}

	p_efecto->num_notas = num_notas;


	return p_efecto->num_notas;
}

// Procedimiento de inicializacion del objeto especifico
// Nota: parte de inicialización común a InicializaPlayDisparo y InicializaPlayImpacto
void InicializaPlayer (TipoPlayer *p_player) {

	p_player->posicion_nota_actual = 0; // Primera nota
	p_player->frecuencia_nota_actual = p_player->p_efecto->frecuencias[0];
	p_player->duracion_nota_actual = p_player->p_efecto->duraciones[0];

	p_player->timerRepro = tmr_new(timer_player_duracion_nota_actual_isr);

	tmr_startms(p_player->timerRepro, p_player->duracion_nota_actual);

	softToneWrite(GPIO_PIN, p_player->frecuencia_nota_actual);
}

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------

int CompruebaStartDisparo (fsm_t* this) {
	int result;

	piLock(PLAYER_FLAGS_KEY);
	result = (flags_player & FLAG_START_DISPARO);
	piUnlock(PLAYER_FLAGS_KEY);

	return result;
}

int CompruebaStartImpacto (fsm_t* this) {
	int result;

	piLock(PLAYER_FLAGS_KEY);
	result = (flags_player & FLAG_START_IMPACTO);
	piUnlock(PLAYER_FLAGS_KEY);

	return result;
}

int CompruebaNuevaNota (fsm_t* this){
	int result;

	piLock(PLAYER_FLAGS_KEY);
	result = (flags_player & FLAG_PLAYER_END);
	piUnlock(PLAYER_FLAGS_KEY);

	return !result;
}

int CompruebaNotaTimeout (fsm_t* this) {
	int result;

	piLock(PLAYER_FLAGS_KEY);
	result = (flags_player & FLAG_NOTA_TIMEOUT);
	piUnlock(PLAYER_FLAGS_KEY);

	return result;
}

int CompruebaFinalEfecto (fsm_t* this) {
	int result;

	piLock(PLAYER_FLAGS_KEY);
	result = (flags_player & FLAG_PLAYER_END);
	piUnlock(PLAYER_FLAGS_KEY);

	return result;
}

//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------

void InicializaPlayDisparo (fsm_t* this) {

	TipoPlayer *p_player;
	TipoEfecto efectoD;
	p_player = (TipoPlayer*) (this->user_data);

	if ((InicializaEfecto(&efectoD, "DISPARO", frecuenciasDisparo, tiemposDisparo, 16) < 0)) {
		piLock(STD_IO_BUFFER_KEY);
		printf("\n[ERROR!!!][InicializaEfecto]\n");
		fflush(stdout);
		piUnlock(STD_IO_BUFFER_KEY);
	}

	p_player->p_efecto = &efectoD;
	InicializaPlayer(p_player);

	piLock(PLAYER_FLAGS_KEY);
	flags_player &= ~FLAG_START_DISPARO;
	piUnlock(PLAYER_FLAGS_KEY);

	digitalWrite(GPIO_PIN_SHOOT, HIGH);

	piLock(STD_IO_BUFFER_KEY);
	printf("\n[PLAYER][InicializaPlayDisparo][NOTA 1][FREC %d][DURA %d]\n",
			p_player->frecuencia_nota_actual, p_player->duracion_nota_actual);
	fflush(stdout);
	piUnlock(STD_IO_BUFFER_KEY);
}

void InicializaPlayImpacto (fsm_t* this) {

	// Creación del player
	TipoPlayer *p_player;
	TipoEfecto efectoP;
	p_player = (TipoPlayer*) (this->user_data);

	if ((InicializaEfecto(&efectoP, "IMPACTO", frecuenciasImpacto, tiemposImpacto, 32) < 0)) {
		piLock(STD_IO_BUFFER_KEY);
		printf("\n[ERROR!!!][InicializaEfecto]\n");
		fflush(stdout);
		piUnlock(STD_IO_BUFFER_KEY);
	}

	p_player->p_efecto = &efectoP;
	InicializaPlayer(p_player);

	piLock(PLAYER_FLAGS_KEY);
	flags_player &= ~FLAG_START_IMPACTO;
	piUnlock(PLAYER_FLAGS_KEY);

	piLock(STD_IO_BUFFER_KEY);
	printf("\n[PLAYER][InicializaPlayImpacto][NOTA 1][FREC %d][DURA %d]\n",
			p_player->frecuencia_nota_actual, p_player->duracion_nota_actual);
	fflush(stdout);
	piUnlock(STD_IO_BUFFER_KEY);
}

void ComienzaNuevaNota (fsm_t* this) {

	// Creación del player.
	TipoPlayer *p_player;
	p_player = (TipoPlayer*) (this->user_data);

	tmr_startms(p_player->timerRepro, p_player->duracion_nota_actual);

	piLock(PLAYER_FLAGS_KEY);
	flags_player &= ~FLAG_PLAYER_END;
	piUnlock(PLAYER_FLAGS_KEY);

	softToneWrite(GPIO_PIN, p_player->frecuencia_nota_actual);

	piLock(STD_IO_BUFFER_KEY);
	printf("\n[PLAYER][ComienzaNuevaNota][NOTA %d][FREC %d][DURA %d]\n",
			p_player->posicion_nota_actual + 1,
			p_player->p_efecto->frecuencias[p_player->posicion_nota_actual],
			p_player->p_efecto->duraciones[p_player->posicion_nota_actual]);
	fflush(stdout);
	piUnlock(STD_IO_BUFFER_KEY);
}

void ActualizaPlayer (fsm_t* this) {

	// Creación del player
	TipoPlayer *p_player;
	p_player = (TipoPlayer*) (this->user_data);

	// Ha terminado el disparo o el impacto
	if ((strcmp(p_player->p_efecto->nombre, "DISPARO") == 0) && (p_player->posicion_nota_actual == 15)) {
		piLock(PLAYER_FLAGS_KEY);
		flags_player |= FLAG_PLAYER_END;
		piUnlock(PLAYER_FLAGS_KEY);

		piLock(STD_IO_BUFFER_KEY);
		printf("\n[PLAYER][ActualizaPlayer][REPRODUCIDAS TODAS LAS NOTAS]\n");
		fflush(stdout);
		piUnlock(STD_IO_BUFFER_KEY);
	} else if ((strcmp(p_player->p_efecto->nombre, "IMPACTO") == 0) && (p_player->posicion_nota_actual == 31)){
		piLock(PLAYER_FLAGS_KEY);
		flags_player |= FLAG_PLAYER_END;
		piUnlock(PLAYER_FLAGS_KEY);

		piLock(STD_IO_BUFFER_KEY);
		printf("\n[PLAYER][ActualizaPlayer][REPRODUCIDAS TODAS LAS NOTAS]\n");
		fflush(stdout);
		piUnlock(STD_IO_BUFFER_KEY);
	// No ha terminado, actualiza
	} else {
		p_player->posicion_nota_actual++;
		if (strcmp(p_player->p_efecto->nombre, "DISPARO") == 0)
			p_player->p_efecto->num_notas = 16;
		else
			p_player->p_efecto->num_notas = 32;

		p_player->frecuencia_nota_actual = p_player->p_efecto->frecuencias[p_player->posicion_nota_actual];
		p_player->duracion_nota_actual = p_player->p_efecto->duraciones[p_player->posicion_nota_actual];
		softToneWrite(GPIO_PIN, p_player->frecuencia_nota_actual);

		piLock(STD_IO_BUFFER_KEY);
		printf("\n[PLAYER][ActualizaPlayer][NUEVA NOTA (%d de %d)]\n",
				p_player->posicion_nota_actual + 1,
				p_player->p_efecto->num_notas);
		fflush(stdout);
		piUnlock(STD_IO_BUFFER_KEY);
	}

	piLock(PLAYER_FLAGS_KEY);
	flags_player &= ~FLAG_NOTA_TIMEOUT;
	piUnlock(PLAYER_FLAGS_KEY);
}

void FinalEfecto (fsm_t* this) {

	piLock(PLAYER_FLAGS_KEY);
	flags_player &= ~FLAG_PLAYER_END;
	piUnlock(PLAYER_FLAGS_KEY);

	digitalWrite(GPIO_PIN_SHOOT, LOW);

	softToneWrite(GPIO_PIN, 0);

	piLock(STD_IO_BUFFER_KEY);
	printf("\n[PLAYER][FinalEfecto]\n");
	printf("\n[FinalEfecto][Flag_PLAYER_END]");
	fflush(stdout);
	piUnlock(STD_IO_BUFFER_KEY);
}
