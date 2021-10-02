#ifndef SHP_GPIO_EXPANDER_H
#define SHP_GPIO_EXPANDER_H


class ShpGpioExpander : public ShpIOPort 
{
	public:
		
		ShpGpioExpander();

		virtual void setPinState(uint8_t pin, uint8_t value);

};


#endif
