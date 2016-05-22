#define 	MAIN_Fosc		22118400L	

#include	"STC15Fxxxx.H"


#define	Timer0_Reload	(65536UL -(MAIN_Fosc / 1000))		

#define DIS_DOT		0x20
#define DIS_BLACK	0x10
#define DIS_		0x11




u8 code t_display[]={
//	 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,
//black	 -     H    J	 K	  L	   N	o   P	 U     t    G    Q    r   M    y
	0x00,0x40,0x76,0x1E,0x70,0x38,0x37,0x5C,0x73,0x3E,0x78,0x3d,0x67,0x50,0x37,0x6e,
	0xBF,0x86,0xDB,0xCF,0xE6,0xED,0xFD,0x87,0xFF,0xEF,0x46};	//0. 1. 2. 3. 4. 5. 6. 7. 8. 9. -1

u8 code T_COM[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

//					  0,1,2,3,4,5,6,7,8,9,10,11,
u8 code key_tab [] = {0,7,8,9,0,4,5,6,0,1,2,3,0,0,0,0,0};

sbit	P_HC595_SER   = P4^0;	//pin 14	SER		data input
sbit	P_HC595_RCLK  = P5^4;	//pin 12	RCLk	store (latch) clock
sbit	P_HC595_SRCLK = P4^3;	//pin 11	SRCLK	Shift data clock


u8 	LED8[8];		
u8	display_index;	
bit	B_1ms;			

u8	ADC_KeyState,ADC_KeyState1,ADC_KeyState2,ADC_KeyState3;	
u8	ADC_KeyHoldCnt;	
u8	KeyCode;	
u8	cnt10ms;

u16	msecond;

u8 state,oper,sign;
long num1,num2,res;

void	CalculateAdcKey(u16 adc);
u16		Get_ADC10bitResult(u8 channel);	//channel = 0~7
void	DisplayRTC(void);
void	RTC(void);

void init(){
	P0M1 = 0;	P0M0 = 0;	
	P1M1 = 0;	P1M0 = 0;	
	P2M1 = 0;	P2M0 = 0;	
	P3M1 = 0;	P3M0 = 0;	
	P4M1 = 0;	P4M0 = 0;	
	P5M1 = 0;	P5M0 = 0;	
	P6M1 = 0;	P6M0 = 0;	
	P7M1 = 0;	P7M0 = 0;	
	
	AUXR = 0x80;	
	TH0 = (u8)(Timer0_Reload / 256);
	TL0 = (u8)(Timer0_Reload % 256);
	ET0 = 1;	//Timer0 interrupt enable
	TR0 = 1;	//Tiner0 run
	EA = 1;		
	
	display_index = 0;
	P1ASF = 0x10;		
	ADC_CONTR = 0xE0;	//90T, ADC power on	
	
	state = 0;
	oper = 0;
	sign = 1;
	
	num1 = 0;
	num2 = 0;
	res = 0;
}


u16	Get_ADC10bitResult(u8 channel)	//channel = 0~7
{
	ADC_RES = 0;
	ADC_RESL = 0;

	ADC_CONTR = (ADC_CONTR & 0xe0) | 0x08 | channel; 	//start the ADC
	NOP(4);

	while((ADC_CONTR & 0x10) == 0)	;	//wait for ADC finish
	ADC_CONTR &= ~0x10;		
	return	(((u16)ADC_RES << 2) | (ADC_RESL & 3));
}

#define	ADC_OFFSET	16
void	CalculateAdcKey(u16 adc)
{
	u8	i;
	u16	j;
	
	if(adc < (64-ADC_OFFSET))
	{
		ADC_KeyState = 0;	
		ADC_KeyHoldCnt = 0;
	}
	j = 64;
	for(i=1; i<=16; i++)
	{
		if((adc >= (j - ADC_OFFSET)) && (adc <= (j + ADC_OFFSET)))	break;	
		j += 64;
	}
	ADC_KeyState3 = ADC_KeyState2;
	ADC_KeyState2 = ADC_KeyState1;
	if(i > 16)	ADC_KeyState1 = 0;	
	else						
	{
		ADC_KeyState1 = i;
		if((ADC_KeyState3 == ADC_KeyState2) && (ADC_KeyState2 == ADC_KeyState1) &&
		   (ADC_KeyState3 > 0) && (ADC_KeyState2 > 0) && (ADC_KeyState1 > 0))
		{
			if(ADC_KeyState == 0)
			{
				KeyCode  = i;	
				ADC_KeyState = i;	
				ADC_KeyHoldCnt = 0;
			}
			if(ADC_KeyState == i)	
			{
				if(++ADC_KeyHoldCnt >= 100)	
				{
					ADC_KeyHoldCnt = 90;
					KeyCode  = i;	
				}
			}
			else	ADC_KeyHoldCnt = 0;	
		}
	}
}



void Send_595(u8 dat)
{		
	u8	i;
	for(i=0; i<8; i++)
	{
		dat <<= 1;
		P_HC595_SER   = CY;
		P_HC595_SRCLK = 1;
		P_HC595_SRCLK = 0;
	}
}


void DisplayScan(void)
{	
	Send_595(~T_COM[display_index]);				
	Send_595(t_display[LED8[display_index]]);	

	P_HC595_RCLK = 1;
	P_HC595_RCLK = 0;							
	if(++display_index >= 8)	display_index = 0;	
}



void timer0 (void) interrupt TIMER0_VECTOR
{
	DisplayScan();	
	B_1ms = 1;		
}

void error()
{
	u8 i;
	for(i=0; i<8; i++)	LED8[i] = 0x10;
	
	state = 0;
	oper = 0;
	sign = 1;
	
	num1 = 0;
	num2 = 0;
	res = 0;
}

void disp(long num)
{
	u8 i;
	if (num == 0)
	{
		LED8[7] = 0;
		return;
	}
	if (num >= 0)
	{
		for (i = 0;i < 8;i++)
			if (num == 0) LED8[7 - i] = 0x10;
			else 
			{
				LED8[7 - i] = num % 10;
				num = num / 10;
			}
	}
	else
	{
		LED8[0] = 0x11;		//negative sign
		num = -num;
		for (i = 0;i < 7;i++)
			if (num == 0) LED8[6 - i] = 0x10;
			else
			{
				LED8[6 - i] = num % 10;
				num = num / 10;
			}
	}
}

void calc()
{
	switch (oper)
	{
		case 1:
			res = num1 + num2;
			break;
		case 2:
			res = num1 - num2;
			break;
		case 3:
			res = num1 * num2;
			break;
		case 4:
			res = num1 / num2;
			break;
	}
}

void react()
{
	switch (KeyCode)
	{
		case 1:case 2:case 3:case 5:case 6:case 7:case 9:case 10:case 11:case 13:
			if (state == 0)
			{
				num1 = num1 * 10 + sign * key_tab[KeyCode];
				disp(num1);
			}
			else if (state == 1)
			{
				num2 = num2 * 10 + sign * key_tab[KeyCode];
				disp(num2);
			}
			break;
		case 8:
			if (state == 0)
			{
				if (num1 == 0) sign = -1;
				else
				{
					oper = KeyCode / 4;
					disp(oper);
					state = 1;
					sign = 1;
				}
			}
			else if (state == 1)
			{
				if (num2 == 0) sign = -1;
				else
				{
					error();
					return;
				}
			}
			break;
		case 4:case 12:case 16:
			if (state == 0)
			{
				oper = KeyCode / 4;
				disp(oper);
				state = 1;
			}
			else
			{
				error();
				return;
			}
			break;
		case 15:
			if (state == 0)
			{
				res = num1;
				disp(res);
				state = 0;
				num1 = 0;
				num2 = 0;
				oper = 0;
				sign = 1;
				res = 0;
			}
			else if (state == 1)
			{
				calc();
				disp(res);
				state = 0;
				num1 = 0;
				num2 = 0;
				sign = 1;
				oper = 0;
				res = 0;
			}
			break;
		default: error();
	}
	KeyCode = 0;
}

void main()
{
	u8	i;
	u16	j;
	
	init();
	
	for(i=0; i<8; i++)	LED8[i] = 0x10;
	
	ADC_KeyState  = 0;
	ADC_KeyState1 = 0;
	ADC_KeyState2 = 0;
	ADC_KeyState3 = 0;	
	ADC_KeyHoldCnt = 0;	
	KeyCode = 0;	
	cnt10ms = 0;
	
	while(1)
	{
		if(B_1ms)	
		{
			B_1ms = 0;

			if(++cnt10ms >= 10)
			{
				cnt10ms = 0;
				j = Get_ADC10bitResult(4);
				if(j < 1024)	CalculateAdcKey(j);
						
			}
			
			//disp(KeyCode);
			if(KeyCode > 0) react();
			
		}
	}
}