/*
 * ============================================================
 * PROJETO: CONTROLE DE CHOCADEIRA DE OVOS
 * ============================================================
 *
 * MICROCONTROLADOR: PIC18F4550
 * COMPILADOR: MPLAB XC8
 *
 * ============================================================
 * COMPONENTES
 * ============================================================
 *
 * RA0/AN0 -> Potenciometro (sensor de temperatura)
 *
 * RD0 -> LED do aquecedor
 * RD1 -> LED do ventilador
 * RD2 -> LED de temperatura normal
 * RD3 -> LED de alarme
 *
 * ============================================================
 * FUNCIONAMENTO
 * ============================================================
 *
 * O potenciometro simula um sensor de temperatura.
 *
 * O ADC converte a tensao do potenciometro em um valor
 * digital de 0 a 1023.
 *
 * Esse valor e convertido para uma temperatura simulada
 * entre 20 e 45 graus Celsius.
 *
 * CONTROLE:
 *
 * Temperatura < 36.5 C
 *      -> Aquecedor ligado
 *
 * 36.5 C <= Temperatura <= 38.0 C
 *      -> Temperatura normal
 *
 * 38.0 C < Temperatura < 39.0 C
 *      -> Ventilador ligado
 *
 * Temperatura >= 39.0 C
 *      -> Ventilador ligado
 *      -> Alarme piscando
 *
 * ============================================================
 * TIMERS
 * ============================================================
 *
 * TIMER1:
 *      Gera uma base de tempo de aproximadamente 1 segundo.
 *
 * TIMER0:
 *      Quando o alarme esta ativo, faz o LED RD3 piscar.
 *
 * ============================================================
 */

#include <xc.h>


/*
 * ============================================================
 * FREQUENCIA DO MICROCONTROLADOR
 * ============================================================
 *
 * Estamos utilizando um cristal de 4 MHz.
 */

#define _XTAL_FREQ 4000000


/*
 * ============================================================
 * CONFIGURACAO DO PIC18F4550
 * ============================================================
 */

#pragma config PLLDIV   = 1
#pragma config CPUDIV   = OSC1_PLL2
#pragma config USBDIV   = 1

#pragma config FOSC     = HS
#pragma config FCMEN    = OFF
#pragma config IESO     = OFF

#pragma config PWRT     = OFF
#pragma config BOR      = OFF
#pragma config VREGEN   = OFF

#pragma config WDT      = OFF

#pragma config MCLRE    = ON
#pragma config LPT1OSC  = OFF
#pragma config PBADEN   = OFF

#pragma config STVREN   = ON
#pragma config LVP      = OFF
#pragma config XINST    = OFF

#pragma config DEBUG    = OFF


/*
 * ============================================================
 * DEFINICAO DOS LEDS
 * ============================================================
 *
 * Utilizamos LATD para controlar as saidas.
 *
 * RD0 -> Aquecedor
 * RD1 -> Ventilador
 * RD2 -> Temperatura normal
 * RD3 -> Alarme
 */

#define LED_AQUECEDOR  LATDbits.LATD0
#define LED_VENTILADOR LATDbits.LATD1
#define LED_NORMAL     LATDbits.LATD2
#define LED_ALARME     LATDbits.LATD3


/*
 * ============================================================
 * VARIAVEIS
 * ============================================================
 */


/*
 * Resultado da conversao do ADC.
 *
 * O ADC possui 10 bits:
 *
 * 0    -> 0 V
 * 1023 -> 5 V
 */

unsigned int valor_adc;


/*
 * Temperatura calculada a partir do ADC.
 */

float temperatura;


/*
 * Contador utilizado pelo Timer1.
 *
 * O Timer1 gera uma interrupcao aproximadamente a cada
 * 100 ms.
 *
 * Depois de 10 interrupcoes temos aproximadamente 1 segundo.
 */

volatile unsigned char contador_timer = 0;


/*
 * Flag utilizada para informar ao programa principal
 * que passou aproximadamente 1 segundo.
 *
 * 0 -> ainda nao passou 1 segundo
 * 1 -> pode realizar uma nova leitura
 */

volatile unsigned char flag_1s = 0;


/*
 * Contador utilizado pelo Timer0 para gerar o pisca
 * do LED de alarme.
 */

volatile unsigned char contador_alarme = 0;


/*
 * Indica se o sistema esta em condicao de alarme.
 *
 * 0 -> alarme desligado
 * 1 -> alarme ativo
 */

volatile unsigned char alarme_ativo = 0;


/*
 * ============================================================
 * INICIALIZACAO DO ADC
 * ============================================================
 *
 * AN0 sera utilizado como entrada analogica.
 *
 * O potenciometro sera conectado em RA0.
 */

void ADC_Init(void)
{
    /*
     * --------------------------------------------------------
     * ADCON1
     * --------------------------------------------------------
     *
     * AN0 = analogico
     * Demais entradas = digitais
     *
     * Vref+ = VDD
     * Vref- = VSS
     */

    ADCON1 = 0x0E;


    /*
     * --------------------------------------------------------
     * ADCON2
     * --------------------------------------------------------
     *
     * ADFM = 1
     * Resultado justificado a direita
     *
     * ACQT = tempo de aquisicao
     *
     * ADCS = clock do ADC
     */

    ADCON2 = 0x92;


    /*
     * --------------------------------------------------------
     * ADCON0
     * --------------------------------------------------------
     *
     * Seleciona AN0.
     * Liga o ADC.
     */

    ADCON0 = 0x01;


    /*
     * RA0 como entrada.
     */

    TRISAbits.TRISA0 = 1;


    /*
     * Pequeno tempo para estabilizacao.
     */

    __delay_ms(10);
}


/*
 * ============================================================
 * LEITURA DO ADC
 * ============================================================
 *
 * Retorna um valor entre 0 e 1023.
 */

unsigned int ADC_Read(void)
{
    unsigned int resultado;


    /*
     * Seleciona o canal AN0.
     */

    ADCON0bits.CHS = 0;


    /*
     * Inicia a conversao.
     */

    ADCON0bits.GO = 1;


    /*
     * Aguarda a conversao terminar.
     *
     * Enquanto GO = 1:
     * ADC ainda esta trabalhando.
     */

    while (ADCON0bits.GO);


    /*
     * Junta os dois registradores do resultado.
     *
     * ADRESH -> parte mais significativa
     * ADRESL -> parte menos significativa
     */

    resultado = ((unsigned int)ADRESH << 8) | ADRESL;


    return resultado;
}


/*
 * ============================================================
 * CONVERSAO DO ADC PARA TEMPERATURA
 * ============================================================
 *
 * Para facilitar a simulacao:
 *
 * 0 V  -> 20 graus Celsius
 * 5 V  -> 45 graus Celsius
 *
 * ADC:
 *
 * 0    -> 20 C
 * 1023 -> 45 C
 *
 * Formula:
 *
 * temperatura = 20 + (ADC * 25 / 1023)
 */

float Converter_Temperatura(unsigned int adc)
{
    float temp;


    temp = 20.0 + ((float)adc * 25.0 / 1023.0);


    return temp;
}


/*
 * ============================================================
 * CONFIGURACAO DO TIMER1
 * ============================================================
 *
 * O Timer1 sera utilizado como base de tempo do sistema.
 *
 * Cristal:
 *      4 MHz
 *
 * Frequencia das instrucoes:
 *      4 MHz / 4 = 1 MHz
 *
 * Cada ciclo:
 *      1 us
 *
 * Prescaler:
 *      1:8
 *
 * Cada incremento:
 *      8 us
 *
 * Para aproximadamente 100 ms:
 *
 * 100000 us / 8 us = 12500 contagens
 *
 * 65536 - 12500 = 53036
 *
 * 53036 = 0xCF2C
 *
 * 10 interrupcoes:
 *
 * 10 x 100 ms = aproximadamente 1 segundo
 */

void Timer1_Init(void)
{
    /*
     * --------------------------------------------------------
     * T1CON
     * --------------------------------------------------------
     *
     * TMR1CS = 0
     * Clock interno (Fosc/4)
     *
     * T1CKPS = 11
     * Prescaler 1:8
     *
     * TMR1ON = 1
     * Timer ligado
     */

    T1CON = 0b00110001;


    /*
     * Carrega o valor inicial.
     */

    TMR1H = 0xCF;
    TMR1L = 0x2C;


    /*
     * Limpa a flag do Timer1.
     */

    PIR1bits.TMR1IF = 0;


    /*
     * Habilita interrupcao do Timer1.
     */

    PIE1bits.TMR1IE = 1;


    /*
     * Habilita interrupcoes dos perifericos.

     */

    INTCONbits.PEIE = 1;


    /*
     * Habilita interrupcoes globais.
     */

    INTCONbits.GIE = 1;
}


/*
 * ============================================================
 * CONFIGURACAO DO TIMER0
 * ============================================================
 *
 * O Timer0 sera utilizado exclusivamente para fazer o LED
 * de alarme piscar.
 *
 * Frequencia do sistema:
 *
 *      Fosc = 4 MHz
 *
 * Frequencia de instrucao:
 *
 *      Fosc/4 = 1 MHz
 *
 * Portanto:
 *
 *      1 ciclo = 1 us
 *
 * Prescaler:
 *
 *      1:256
 *
 * Cada incremento do Timer0:
 *
 *      256 us
 *
 * O Timer0 e carregado com:
 *
 *      0xFF3D
 *
 * Isso gera aproximadamente 50 ms entre interrupcoes.
 *
 * 10 interrupcoes:
 *
 *      10 x 50 ms = aproximadamente 500 ms
 *
 * Portanto o LED troca de estado aproximadamente a cada
 * meio segundo.
 */

void Timer0_Init(void)
{
    /*
     * --------------------------------------------------------
     * T0CON
     * --------------------------------------------------------
     *
     * TMR0ON = 1
     * Timer0 ligado
     *
     * T08BIT = 0
     * Timer0 em 16 bits
     *
     * T0CS = 0
     * Clock interno
     *
     * PSA = 0
     * Prescaler utilizado
     *
     * T0PS = 111
     * Prescaler 1:256
     */

    T0CON = 0b10000111;


    /*
     * Carrega o valor inicial.
     */

    TMR0H = 0xFF;
    TMR0L = 0x3D;


    /*
     * Limpa a flag do Timer0.
     */

    INTCONbits.TMR0IF = 0;


    /*
     * Habilita interrupcao do Timer0.
     */

    INTCONbits.TMR0IE = 1;
}


/*
 * ============================================================
 * CONTROLE DA CHOCADEIRA
 * ============================================================
 *
 * Esta funcao decide qual atuador/indicador deve estar ativo
 * de acordo com a temperatura medida.
 */

void Controlar_Chocadeira(float temp)
{
    /*
     * ========================================================
     * TEMPERATURA BAIXA
     * ========================================================
     *
     * Abaixo de 36.5 C:
     *
     * -> Liga aquecedor
     * -> Desliga ventilador
     * -> Desliga indicador normal
     * -> Desativa alarme
     */

    if (temp < 36.5)
    {
        LED_AQUECEDOR = 1;

        LED_VENTILADOR = 0;

        LED_NORMAL = 0;

        alarme_ativo = 0;
    }


    /*
     * ========================================================
     * TEMPERATURA NORMAL
     * ========================================================
     *
     * Entre 36.5 C e 38.0 C:
     *
     * -> Aquecedor desligado
     * -> Ventilador desligado
     * -> LED normal ligado
     * -> Alarme desligado
     */

    else if (temp <= 38.0)
    {
        LED_AQUECEDOR = 0;

        LED_VENTILADOR = 0;

        LED_NORMAL = 1;

        alarme_ativo = 0;
    }


    /*
     * ========================================================
     * TEMPERATURA ALTA
     * ========================================================
     *
     * Acima de 38 C e abaixo de 39 C:
     *
     * -> Aquecedor desligado
     * -> Ventilador ligado
     * -> LED normal desligado
     * -> Alarme desligado
     */

    else if (temp < 39.0)
    {
        LED_AQUECEDOR = 0;

        LED_VENTILADOR = 1;

        LED_NORMAL = 0;

        alarme_ativo = 0;
    }


    /*
     * ========================================================
     * SUPERAQUECIMENTO
     * ========================================================
     *
     * Temperatura >= 39 C:
     *
     * -> Aquecedor desligado
     * -> Ventilador ligado
     * -> LED normal desligado
     * -> Ativa o sistema de alarme
     *
     * IMPORTANTE:
     *
     * Aqui nao ligamos diretamente o LED_ALARME.
     *
     * Apenas ativamos a variavel alarme_ativo.
     *
     * O Timer0 sera responsavel por fazer o LED piscar.
     */

    else
    {
        LED_AQUECEDOR = 0;

        LED_VENTILADOR = 1;

        LED_NORMAL = 0;

        alarme_ativo = 1;
    }
}


/*
 * ============================================================
 * ROTINA DE INTERRUPCAO
 * ============================================================
 *
 * Temos dois Timers:
 *
 * TIMER1:
 *      Base de tempo de aproximadamente 100 ms.
 *
 *      Depois de 10 interrupcoes:
 *      -> gera uma flag de 1 segundo.
 *
 *
 * TIMER0:
 *      Base de tempo de aproximadamente 50 ms.
 *
 *      Depois de 10 interrupcoes:
 *      -> inverte o LED de alarme.
 */

void __interrupt() interrupcao(void)
{
    /*
     * ========================================================
     * TIMER1
     * ========================================================
     */

    if (PIR1bits.TMR1IF)
    {
        /*
         * Recarrega o Timer1 para aproximadamente 100 ms.
         */

        TMR1H = 0xCF;
        TMR1L = 0x2C;


        /*
         * Limpa a flag.
         */

        PIR1bits.TMR1IF = 0;


        /*
         * Incrementa o contador.
         */

        contador_timer++;


        /*
         * Quando chegamos a 10 interrupcoes:
         *
         * 10 x 100 ms = aproximadamente 1 segundo
         */

        if (contador_timer >= 10)
        {
            contador_timer = 0;


            /*
             * Informa ao programa principal que passou
             * aproximadamente 1 segundo.
             */

            flag_1s = 1;
        }
    }


    /*
     * ========================================================
     * TIMER0
     * ========================================================
     *
     * Responsavel pelo pisca do LED de alarme.
     */

    if (INTCONbits.TMR0IF)
    {
        /*
         * Recarrega Timer0 para aproximadamente 50 ms.
         */

        TMR0H = 0xFF;
        TMR0L = 0x3D;


        /*
         * Limpa a flag.
         */

        INTCONbits.TMR0IF = 0;


        /*
         * Verifica se o alarme esta ativo.
         */

        if (alarme_ativo)
        {
            /*
             * Conta as interrupcoes do Timer0.
             */

            contador_alarme++;


            /*
             * Aproximadamente:
             *
             * 10 x 50 ms = 500 ms
             */

            if (contador_alarme >= 10)
            {
                /*
                 * Zera o contador.
                 */

                contador_alarme = 0;


                /*
                 * Inverte o estado do LED.
                 *
                 * Se estiver:
                 *
                 * 0 -> vira 1
                 * 1 -> vira 0
                 */

                LED_ALARME = !LED_ALARME;
            }
        }


        /*
         * ====================================================
         * SEM ALARME
         * ====================================================
         *
         * Se a temperatura voltou ao normal:
         *
         * -> Zera contador
         * -> Desliga LED de alarme
         */

        else
        {
            contador_alarme = 0;

            LED_ALARME = 0;
        }
    }
}


/*
 * ============================================================
 * FUNCAO PRINCIPAL
 * ============================================================
 */

void main(void)
{
    /*
     * ========================================================
     * CONFIGURACAO DOS PINOS
     * ========================================================
     *
     * PORTD inteiro como saida.
     *
     * RD0 -> Aquecedor
     * RD1 -> Ventilador
     * RD2 -> Normal
     * RD3 -> Alarme
     */

    TRISD = 0x00;


    /*
     * Inicialmente todos os LEDs desligados.
     */

    LATD = 0x00;


    /*
     * ========================================================
     * INICIALIZACAO DO ADC
     * ========================================================
     */

    ADC_Init();


    /*
     * ========================================================
     * INICIALIZACAO DO TIMER1
     * ========================================================
     *
     * Timer1 sera a base de tempo principal.
     */

    Timer1_Init();


    /*
     * ========================================================
     * INICIALIZACAO DO TIMER0
     * ========================================================
     *
     * Timer0 sera utilizado para piscar o LED de alarme.
     */

    Timer0_Init();


    /*
     * ========================================================
     * LOOP PRINCIPAL
     * ========================================================
     */

    while (1)
    {
        /*
         * ----------------------------------------------------
         * O Timer1 gera uma flag aproximadamente a cada
         * 1 segundo.
         *
         * Somente quando essa flag estiver ativa fazemos
         * uma nova leitura do sensor.
         * ----------------------------------------------------
         */

        if (flag_1s)
        {
            /*
             * Limpa a flag.
             */

            flag_1s = 0;


            /*
             * ------------------------------------------------
             * 1. LEITURA DO POTENCIOMETRO
             * ------------------------------------------------
             *
             * O ADC converte a tensao em um numero de 0 a 1023.
             */

            valor_adc = ADC_Read();


            /*
             * ------------------------------------------------
             * 2. CONVERSAO PARA TEMPERATURA
             * ------------------------------------------------
             */

            temperatura = Converter_Temperatura(valor_adc);


            /*
             * ------------------------------------------------
             * 3. CONTROLE DA CHOCADEIRA
             * ------------------------------------------------
             */

            Controlar_Chocadeira(temperatura);
        }
    }
}