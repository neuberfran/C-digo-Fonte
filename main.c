// -----------------------------------------------------
// INCLUS�O DE ARQUIVOS HEADER
// -----------------------------------------------------
#include "main.h"

// Configura��es do PIC
#pragma config OSC = HSPLL
#pragma config OSCS = OFF
#pragma config PWRT = OFF
#pragma config BOR = OFF
#pragma config WDT = OFF
#pragma config WDTPS = 128
#pragma config STVR = OFF
#pragma config LVP = OFF

#define Pino_CTS PORTCbits.RC4
#define Botao_E  PORTCbits.RC3
#define Botao_D  PORTCbits.RC2
#define LED 	 PORTCbits.RC1
#define Botao_R  PORTAbits.RA2

// -----------------------------------------------------
// PROT�TIPOS DAS FUN��ES
// -----------------------------------------------------
void Tratamento_Interrupcao_Low(void);
void Tratamento_Interrupcao_High(void);
void inicializar(void);
void Enviar(void);


// -----------------------------------------------------
// DECLARA��O DE VARI�VEIS GLOBAIS
// -----------------------------------------------------
unsigned char Atualizar_Display = 0;
unsigned char Valor_Lado_E = 0;
unsigned char Valor_Lado_D = 0;
extern rom unsigned char Numeros[21][975];

// -----------------------------------------------------
// POSICIONAMENTO DOS VETORES DE INTERRUP��O
// -----------------------------------------------------
#pragma code Interrupcao_High = 0x008   // Interrup��o alta
void Interrupcao_High (void)
{
	_asm
    	goto Tratamento_Interrupcao_High
  	_endasm
}
#pragma code

// -----------------------------------------------------
// FUN��O PRINCIPAL - MAIN
// -----------------------------------------------------
void main(void)
{
	inicializar();										// Inicializa��o do sistema

	// Delays de inicializa��o
	Delay1KTCYx(250);
	Delay1KTCYx(250);
	Delay1KTCYx(250);
	Delay1KTCYx(250);
	Delay1KTCYx(250);
	Delay1KTCYx(250);

	// Inicializa��o dos valores dos bot�es
	Valor_Lado_E = 0;
	Valor_Lado_D = 0;

	Atualizar_Display = 0;


	// Sequencia de comandos para limpar a tela
	while(BusyUSART());
	while(Pino_CTS != 0);
	putcUSART(0x1B);

	while(BusyUSART());
	while(Pino_CTS != 0);
	putcUSART(0x5B);

	while(BusyUSART());
	while(Pino_CTS != 0);
	putcUSART(0x32);

	while(BusyUSART());
	while(Pino_CTS != 0);
	putcUSART(0x4A);

	// Envia os valores atuais (0 e 0) para o monitor
	Enviar();

	while (1)
	{
		if (Atualizar_Display)
		{
			Enviar();
			Atualizar_Display = 0;
		}
		Nop();
	}
}


// -----------------------------------------------------
// FUN��O DE INICIALIZA��O DO PIC
// -----------------------------------------------------
void inicializar(void)
{
	ADCON0=0x00;								// Configura��es do AD do PIC
	ADCON1=0x06;
	CCP1CON=0x00;

	TRISA = 0x00;
	TRISAbits.TRISA2 = 1;				// Entrada Bot�o Reset
	TRISB = 0x00;
	TRISC = 0x00;
	TRISCbits.TRISC2 = 1;				// Entrada Bot�o D
	TRISCbits.TRISC3 = 1;				// Entrada Bot�o E
	TRISCbits.TRISC4 = 1;				// CTS do PIC = RTS do VGA
	TRISCbits.TRISC7 = 0;				// RX do PIC - n�o utilizada, deixada como output

	PORTA = 0x00;
	PORTB = 0x00;
	PORTC = 0x00;

	OpenUSART( USART_TX_INT_OFF & 
	           USART_RX_INT_OFF & 
               USART_ASYNCH_MODE & 
               USART_EIGHT_BIT &
               USART_CONT_RX &
	           USART_BRGH_HIGH ,1);	//129 = 19.200, 4 = 500.000, 1 = 1.250.000

	while  (BusyUSART());

	OpenTimer0( TIMER_INT_ON &		// Configura��o do timer 0 para ciclo de 10ms (40MHz)
				T0_16BIT &
				T0_SOURCE_INT &
				T0_PS_1_8);

	TMR0L = 0;
	TMR0H = 0;
	INTCONbits.TMR0IF = 0;	

	INTCONbits.PEIE = 1;			// enable all peripheral interrupt sources
	INTCONbits.GIE = 1;				// enable all interrupt sources

	WriteTimer0 ( 53035 );

}


// -----------------------------------------------------
// FUN��O DE ENVIO DE VALORES PARA O DISPLAY
// -----------------------------------------------------
void Enviar(void)
{
	unsigned int i=0;
	unsigned int Controle_Linha = 0;
	unsigned char Linha=1;

	INTCONbits.PEIE = 0;			// disable all peripheral interrupt sources
	INTCONbits.GIE = 0;				// disable all interrupt sources

	while (Linha < 26)
	{
		while(BusyUSART());
		while(Pino_CTS != 0);
		putcUSART(0x20);	// Envia um espa�o

		for (i=Controle_Linha;i<(39+Controle_Linha);i++)
		{
			while(BusyUSART());
			while(Pino_CTS != 0);
			putcUSART(Numeros[Valor_Lado_E][i]);
		}

		while(BusyUSART());
		while(Pino_CTS != 0);
		putcUSART(0x20);	// Tracejado 0xDB
	
		for (i=Controle_Linha;i<(39+Controle_Linha);i++)
		{
			while(BusyUSART());
			while(Pino_CTS != 0);
			putcUSART(Numeros[Valor_Lado_D][i]);	
		}
	
		Controle_Linha+=39;
		Linha++;
	}
	INTCONbits.PEIE = 1;			// enable all peripheral interrupt sources
	INTCONbits.GIE = 1;				// enable all interrupt sources
}


// -----------------------------------------------------
// �REA DE TRATAMENTO DAS INTERRUP��ES
// -----------------------------------------------------
#pragma code
#pragma interrupt Tratamento_Interrupcao_High
void Tratamento_Interrupcao_High()
{
	static unsigned char Controle_Led = 0;
	static unsigned char Aciona_D = 0;
	static unsigned char Aciona_E = 0;
	static unsigned char Aciona_R = 0;
	static unsigned char Debounce_Aciona_Valor_D = 5;
	static unsigned char Debounce_Aciona_Valor_E = 5;
	static unsigned char Debounce_Aciona_Valor_R = 5;
	static unsigned char Debounce_Desaciona_Valor_D = 5;
	static unsigned char Debounce_Desaciona_Valor_E = 5;
	static unsigned char Debounce_Desaciona_Valor_R = 5;

	if (INTCONbits.TMR0IF)														
	{																			
		INTCONbits.TMR0IF = 0;													
		WriteTimer0 ( 53035 );

		Controle_Led++;
		if (Controle_Led >= 100)
		{
			Controle_Led = 0;
			LED = !LED;
		}

		// Bot�o R
		if (Botao_R && (Debounce_Aciona_Valor_R > 0) && (!Aciona_R))
		{
			Debounce_Aciona_Valor_R--;
		}
		else if (Botao_R && (Debounce_Aciona_Valor_R == 0) && (!Aciona_R))
		{
			Valor_Lado_D = 0;
			Valor_Lado_E = 0;
			Atualizar_Display = 1;

			Debounce_Aciona_Valor_R = 5;
			Aciona_R = 1;
		}
		else
			Debounce_Aciona_Valor_R = 5;

		if (!Botao_R && (Debounce_Desaciona_Valor_R > 0) && (Aciona_R))
		{
			Debounce_Desaciona_Valor_R--;
		}		
		else if (!Botao_R && (Debounce_Desaciona_Valor_R == 0) && (Aciona_R))
		{
			Debounce_Desaciona_Valor_R = 5;
			Aciona_R = 0;
		}

		// Bot�o E
		if (Botao_E && (Debounce_Aciona_Valor_E > 0) && (!Aciona_E))
		{
			Debounce_Aciona_Valor_E--;
		}
		else if (Botao_E && (Debounce_Aciona_Valor_E == 0) && (!Aciona_E))
		{
			if (Valor_Lado_E < 20)
			{
				Valor_Lado_E++;
				Atualizar_Display = 1;
			}
			Debounce_Aciona_Valor_E = 5;
			Aciona_E = 1;
		}
		else
			Debounce_Aciona_Valor_E = 5;

		if (!Botao_E && (Debounce_Desaciona_Valor_E > 0) && (Aciona_E))
		{
			Debounce_Desaciona_Valor_E--;
		}		
		else if (!Botao_E && (Debounce_Desaciona_Valor_E == 0) && (Aciona_E))
		{
			Debounce_Desaciona_Valor_E = 5;
			Aciona_E = 0;
		}


		// Bot�o D
		if (Botao_D && (Debounce_Aciona_Valor_D > 0) && (!Aciona_D))
		{
			Debounce_Aciona_Valor_D--;
		}
		else if (Botao_D && (Debounce_Aciona_Valor_D == 0) && (!Aciona_D))
		{
			if (Valor_Lado_D < 20)
			{
				Valor_Lado_D++;
				Atualizar_Display = 1;
			}
			Debounce_Aciona_Valor_D = 5;
			Aciona_D = 1;
		}
		else
			Debounce_Aciona_Valor_D = 5;

		if (!Botao_D && (Debounce_Desaciona_Valor_D > 0) && (Aciona_D))
		{
			Debounce_Desaciona_Valor_D--;
		}		
		else if (!Botao_D && (Debounce_Desaciona_Valor_D == 0) && (Aciona_D))
		{
			Debounce_Desaciona_Valor_D = 5;
			Aciona_D = 0;
		}
	}
}
