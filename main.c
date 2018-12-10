// MAX7219_CLK   GPIO_Pin_0 //PORTA
// MAX7219_CS    GPIO_Pin_1 //PORTA
// MAX7219_DIN   GPIO_Pin_2 //PORTA


#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
//#include "stm32f10x_adc.h"
//#include "stm32f10x_tim.h"
#include "stm32f10x_nvic.h"
//#include "stm32f10x_spi.h"
#include "stdio.h"
//#include "misc.h"

#define uchar unsigned char
#define ushort unsigned short
#define ulong unsigned long

#define MAX7219_CLK   GPIO_Pin_0    //PORTA
#define MAX7219_CS    GPIO_Pin_2    //PORTA
#define MAX7219_DIN   GPIO_Pin_1    //PORTA
#define LED_BEAT      GPIO_Pin_13   //PORTC


ErrorStatus HSEStartUpStatus;
GPIO_InitTypeDef GPIO_InitStructure;
//void RCC_Configuration(void);

void SetSysClockTo72(void)
{
    ErrorStatus HSEStartUpStatus;
    /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration -----------------------------*/
    /* RCC system reset(for debug purpose) */
    RCC_DeInit();

    /* Enable HSE */
    RCC_HSEConfig( RCC_HSE_ON);

    /* Wait till HSE is ready */
    HSEStartUpStatus = RCC_WaitForHSEStartUp();

    if (HSEStartUpStatus == SUCCESS)
    {
        /* Enable Prefetch Buffer */
        //FLASH_PrefetchBufferCmd( FLASH_PrefetchBuffer_Enable);

        /* Flash 2 wait state */
        //FLASH_SetLatency( FLASH_Latency_2);

        /* HCLK = SYSCLK */
        RCC_HCLKConfig( RCC_SYSCLK_Div1);

        /* PCLK2 = HCLK */
        RCC_PCLK2Config( RCC_HCLK_Div1);

        /* PCLK1 = HCLK/2 */
        RCC_PCLK1Config( RCC_HCLK_Div2);

        /* PLLCLK = 8MHz * 9 = 72 MHz */
        RCC_PLLConfig(0x00010000, RCC_PLLMul_9);

        /* Enable PLL */
        RCC_PLLCmd( ENABLE);

        /* Wait till PLL is ready */
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
        {
        }

        /* Select PLL as system clock source */
        RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK);

        /* Wait till PLL is used as system clock source */
        while (RCC_GetSYSCLKSource() != 0x08)
        {
        }
    }
    else
    { /* If HSE fails to start-up, the application will have wrong clock configuration.
                     User can add here some code to deal with this error */

        /* Go to infinite loop */
        while (1)
        {
        }
    }
}

uchar const LEDcode[]=
{
    0x7e,0x30,0x6d,0x79,0x33,0x5b,0x5f,0x70,0x7f,0x7b, //0..9
    0x7d,0x1f,0x0d,0x3d,0x6f,0x47, //a..f
    0x67, //P-16
    0x3e, //U-17
    0x4f, //E-18
    0x4e, //C-19
    0x77, //A-20
    0x37, //H-21
    0x38, //J-22
    0x0e, //L-23
    0x1D, //o-24
    0x73, //q-25
    0x05, //r-26
    0x0f, //t-27
    0x3e, //U-28
    0x1c, //u-29
    0x3b, //Y-30
    0x17, //h-31
    0x80, //decimal point-32
};

void delay_ms(uint ms)
{
    uint i;
    uint j;
    for (j=0; j<ms; j++) for (i=0; i<8000; i++);
}

void IO_Init(void)
{
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC , ENABLE);

    //Indicator LED
    GPIO_InitStructure.GPIO_Pin = LED_BEAT;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    //MAX7219
    GPIO_InitStructure.GPIO_Pin = MAX7219_CLK | MAX7219_DIN | MAX7219_CS;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}


void MAX7219_SendByte (uchar dat)
{
    uchar i;

    for (i=0;i<8;i++)
    {
        if((dat&0x80)==0x80) GPIO_SetBits(GPIOA, MAX7219_DIN);
        else GPIO_ResetBits(GPIOA, MAX7219_DIN);
        GPIO_ResetBits(GPIOA, MAX7219_CLK);
        GPIO_SetBits(GPIOA, MAX7219_CLK);
        dat<<=1;
    }
}

void MAX7219_SendAddrDat (uchar addr, uchar dat)
{
    GPIO_ResetBits(GPIOA, MAX7219_CS);
    MAX7219_SendByte (addr);
    MAX7219_SendByte (dat);
    GPIO_SetBits(GPIOA, MAX7219_CS);
}

void MAX7219_Init (void)
{
    MAX7219_SendAddrDat (0x0c,0x01); //normal operation
    MAX7219_SendAddrDat (0x0a,0x04); //intensity default=9
    MAX7219_SendAddrDat (0x0b,0x07); //all digits on
    MAX7219_SendAddrDat (0x09,0x00); //decoding
    MAX7219_SendAddrDat (0x0f,0x00); //display test off
}


void MAX7219_Clear(void)
{
    uchar i;
    for(i=8;i>0;i--) MAX7219_SendAddrDat(i,0x00);
}




void MAX7219_DisplayInt (uint val)
{
    u16 a;
    u32 devider;
    u8 i;

    a=val;
    devider=10000000;
    for (i=0; i<8; i++)
    {
        MAX7219_SendAddrDat(8-i,LEDcode[(a/devider)]);
        a=a%devider;
        devider/=10;
    }
}


int main(void)
{

    u32 count=0;
    u8 count8bit=0;
    SetSysClockTo72();

    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);

    IO_Init();
    MAX7219_Init ();
    MAX7219_Clear();
    MAX7219_DisplayInt (count);

    while (1)
    {
        count++;
        if (count>99999999) count=0;

        //display 8 digits on seven segment
        MAX7219_Init ();
        //MAX7219_DisplayInt (count);
        MAX7219_SendAddrDat(1,LEDcode[count8bit]);
        count8bit++;
        if (count8bit>32) count8bit=0;

        //blink the LED on mcu board
        GPIO_WriteBit( GPIOC, LED_BEAT, (BitAction) !GPIO_ReadInputDataBit(GPIOC, LED_BEAT) );

        delay_ms(500);
    }
}


