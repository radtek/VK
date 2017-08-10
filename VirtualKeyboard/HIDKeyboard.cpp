#include "stdafx.h"
#include "HIDKeyboard.h"


HIDKeyboard::HIDKeyboard()
{
}
bool HIDKeyboard::isModifier(keyType key)
{
	if (key >= 0x80 && key <= 0x87)
		return true;
	else
		return false;
}
void HIDKeyboard::push(keyType key)
{
	if (isModifier(key))
	{
		report.modifiers |= (1 << (key - 128));
		return;
	}
		for (auto &k : report.keys)
		{
			if (key == k)
				return;
		}
	//add to already pressed
	for (auto &k : report.keys)
	{
		if (k == 0x00)
		{
			k = key;
			break;
		}
	}

}

void HIDKeyboard::release(keyType key)
{
	if (isModifier(key))
	{
		report.modifiers &= ~(1 << (key - 128));
		return;
	}
	for (auto &k : report.keys)
	{
		if (key == k)
			k=0x00;
	}
}

void HIDKeyboard::releaseAll()
{
	report.modifiers = 0;
	for(auto &k:report.keys)
	{
		k = 0;
	}
}



