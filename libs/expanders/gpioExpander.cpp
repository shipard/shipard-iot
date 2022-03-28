
ShpGpioExpander::ShpGpioExpander()
{
	for (uint8_t i = 0; i < SHP_GPIO_EXP_MAX_PINS; i++)
		m_inputIOPorts[i] = NULL;
}

void ShpGpioExpander::setInputPinIOPort(uint8_t pin, ShpInputBinary *ioPort)
{
	m_inputIOPorts[pin] = ioPort;
}

void ShpGpioExpander::setPinState(uint8_t pin, uint8_t value)
{
}
