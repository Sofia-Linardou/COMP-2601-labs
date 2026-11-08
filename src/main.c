#include <stdint.h>
#include <stm32l031xx.h>

#define TIMER_TICK_HZ 1000000UL  // TIM2 counter runs at 1 MHz

void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void delay(volatile uint32_t dly);
void initClock(void);
void initTimer(void);
int btn1pressed(void);
int btn2pressed(void);
void playNote(uint32_t Freq);
void greenOn(void);   
void yellowOn(void);   
void redOn(void);   
void greenOff(void);   
void yellowOff(void);   
void redOff(void);   
void trafficLights(void) ;

int main(void)
{
    initClock();
    RCC->IOPENR |= RCC_IOPENR_IOPAEN | RCC_IOPENR_IOPBEN; // enable ports A and B

    pinMode(GPIOA, 0, 1); // Make GPIOA bit 0 an output GREEN LED
    pinMode(GPIOA, 1, 1); // Make GPIOA bit 1 an output YELLOW LED
    pinMode(GPIOA, 2, 1); // Make GPIOA bit 2 an output RED LED

    pinMode(GPIOB, 3, 1); // Make GPIOB bit 3 an output 
    pinMode(GPIOB, 4, 0); // Make GPIOB bit 4 an input
    enablePullUp(GPIOB, 4);
    pinMode(GPIOB, 5, 0); // Make GPIOA bit 5 an input
    enablePullUp(GPIOB, 5);

    initTimer();

    while (1)
    {
        if (btn1pressed())
        {
            playNote(1000);           // Play a 1 kHz tone on PB1
            trafficLights();  
            while (btn1pressed());   // Wait until button is released
        }
        playNote(0);                  // Stop the sound
    }
}

int btn1pressed(void)
{
    if ((GPIOB->IDR & (1u << 5)) == 0u)
        return 1;
    else
        return 0;
}

int btn2pressed(void)
{
    if ((GPIOB->IDR & (1u << 4)) == 0u)
        return 1;
    else
        return 0;
}

void playNote(uint32_t Freq)
{
    // TIM2 channel 4 outputs PWM on PB1. The timer counter runs at 1 MHz.
    if (Freq == 0u)
    {
        TIM2->CCR4 = 0u;         // 0% duty cycle: keep output low
        TIM2->EGR = TIM_EGR_UG;  // Apply the updated compare value
        TIM2->CR1 &= ~TIM_CR1_CEN;
        return;
    }

    uint32_t period = TIMER_TICK_HZ / Freq;
    if (period < 2u) period = 2u;
    if (period > 65536u) period = 65536u;

    TIM2->CR1 &= ~TIM_CR1_CEN;
    TIM2->ARR = period - 1u;     // PWM period = (ARR + 1) timer ticks
    TIM2->CCR4 = period / 2u;   // 50% duty cycle
    TIM2->CNT = 0u;
    TIM2->EGR = TIM_EGR_UG;     // Load PSC/ARR/CCR4 immediately
    TIM2->CR1 |= TIM_CR1_CEN;
}

void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber)
{
    Port->PUPDR &= ~(3u << (BitNumber * 2u));
    Port->PUPDR |=  (1u << (BitNumber * 2u));
}

void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
    uint32_t mode_value = Port->MODER;
    Mode <<= (2u * BitNumber);
    mode_value &= ~(3u << (BitNumber * 2u));
    mode_value |= Mode;
    Port->MODER = mode_value;
}

void delay(volatile uint32_t dly)
{
    while (dly--);
}

void initClock(void)
{
    // The STM32L031K6 supports 16 MHz HSI; it does not support the
    // STM32F031's 48 MHz clock configuration.
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0u);

    RCC->CR &= ~RCC_CR_HSIDIVEN; // Use HSI16, not HSI16/4
    while ((RCC->CR & RCC_CR_HSIDIVF) != 0u);

    // Set AHB and APB prescalers to 1, then select HSI16 as system clock.
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);
}

void initTimer(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 peripheral clock

    pinMode(GPIOB, 1, 2);             // PB1: alternate-function mode
    GPIOB->AFR[0] &= ~(0xFu << 4);    // Clear AF setting for PB1
    GPIOB->AFR[0] |=  (5u << 4);      // PB1 AF5 = TIM2_CH4 on L031K6

    TIM2->CR1 = TIM_CR1_ARPE;         // Enable auto-reload preload, stopped
    TIM2->CCMR2 = TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2 |
                  TIM_CCMR2_OC4PE;    // Channel 4 PWM mode 1, preload on
    TIM2->CCER = TIM_CCER_CC4E;      // Enable TIM2 channel 4 output
    TIM2->PSC = 16u - 1u;            // 16 MHz / 16 = 1 MHz timer counter
    TIM2->ARR = 1000u - 1u;
    TIM2->CCR4 = 0u;                 // Silent until playNote() is called
    TIM2->CNT = 0u;
    TIM2->EGR = TIM_EGR_UG;          // Apply prescaler and preload values
}

void trafficLights(void) 
{ 
    delay(200000); 
    greenOff();   
    yellowOn();   
    delay(200000);   
    yellowOff();   
    redOn();   
    delay(400000);   
    redOff();  
    greenOn();
} 

void greenOn()   
{   
    uint32_t original;   
    original = GPIOA->ODR;   
    original = original | 1;   
    GPIOA->ODR = original;   
}

void yellowOn()   
{   
    uint32_t original;   
    original = GPIOA->ODR;   
    original = original | 2;   
    GPIOA->ODR = original;   
}

void redOn()   
{   
    uint32_t original;   
    original = GPIOA->ODR;   
    original = original | 4;   
    GPIOA->ODR = original;   
}   

void greenOff()   
{   
    uint32_t original;   
    original = GPIOA->ODR;   
    original = original & ~(1);   
    GPIOA->ODR = original;   
}   

void yellowOff()   
{   
    uint32_t original;   
    original = GPIOA->ODR;   
    original = original & ~(2);   
    GPIOA->ODR = original;   
}   

void redOff()   
{   
    uint32_t original;   
    original = GPIOA->ODR;   
    original = original & ~(4);   
    GPIOA->ODR = original;   
} 