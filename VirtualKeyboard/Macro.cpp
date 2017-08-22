#include "stdafx.h"
#include "Macro.h"


Macro::Macro(BlockingQueue<std::shared_ptr<IMessage>> &que):bque{que}
{
	keyReport.id = 1;
	mouseReport.id = 2;
	mp["delayS"] = std::bind(&Macro::sleepS, this, std::placeholders::_1); //&Macro::sleepS;
	mp["delayMS"] = std::bind(&Macro::sleepMS, this, std::placeholders::_1);
	mp["keyboardPush"] = std::bind(&Macro::keyboardPush, this, std::placeholders::_1);
	mp["keyboardRelease"] = std::bind(&Macro::keyboardRelease, this, std::placeholders::_1);
	mp["keyboardReleaseAll"] = std::bind(&Macro::keyboardReleaseAll, this, std::placeholders::_1);
	mp["mouseMove"] = std::bind(&Macro::mouseMove, this, std::placeholders::_1);
	mp["mouseReleaseAll"] = std::bind(&Macro::mouseReleaseAll, this, std::placeholders::_1);
	mp["mouseRelease"] = std::bind(&Macro::mouseRelease, this, std::placeholders::_1);
	mp["mousePush"] = std::bind(&Macro::mousePush, this, std::placeholders::_1);
}

int Macro::load(std::string const fname)
{
	try {
		file.open(fname, std::fstream::in);
		std::getline(file, buffer);
		Format format = ParseHeader(buffer);
		if (format == Format::Long) {
			ReadFileInLongFormat();
		}
		else
			ReadFileInShortFormat();
	}
	catch(std::exception &ex)
	{
		PrintThread{} << ex.what();
		exit(1);
	}
	return 0;
}

void Macro::sleepS(std::string l)
{
	unsigned int delay;
	try {
		delay = boost::lexical_cast<unsigned int>(l);
	}
	catch (const boost::bad_lexical_cast& ex)
	{
		throw ParseException{ "Wrong parameter" };
	}
	boost::this_thread::sleep(boost::posix_time::seconds(delay));
}

void Macro::sleepMS(std::string l)
{
	unsigned int delay;
	try {
		delay = boost::lexical_cast<unsigned int>(l);
	}
	catch (const boost::bad_lexical_cast& ex)
	{
		throw ParseException{ "Wrong parameter" };
	}
	boost::this_thread::sleep(boost::posix_time::millisec(delay));
}

void Macro::keyboardPush(std::string l)
{
	auto k = KeyCodeFromString(l);
	if (isNonPrintable(k)) //non printable
	{
		k = k - 136;
	}
	else if (isAModifier(k)) {	// modifier
		keyReport.modifiers |= (1 << (k - 128));
		k = 0;
	}
	else {	// printable
		k = keymap[k];
		if (isPrintableReachedWithShift(k)) {// character reached with shift
			keyReport.modifiers |= 0x02;	// the left shift modifier
			k = stripModifiers(k); //limit to ascii(get rid of a modifier)
		}
	}
	addToReport(k);
	auto tp = std::make_shared<Report>(keyReport);
	bque.push(tp);
}

size_t Macro::keyboardRelease(std::string l) {
	uint8_t k = KeyCodeFromString(l);
	if (isNonPrintable(k))
	{
		k = k - 136;
	}
	else if (isAModifier(k)) {	// modifier
		keyReport.modifiers &= ~(1 << (k - 128));
		k = 0;
	}
	else {	// printable
		k = keymap[k];
		if (!k)
			return 0;
		if (isPrintableReachedWithShift(k)) {// character reached with shift
			keyReport.modifiers &= ~(0x02);	// the left shift modifier
			k = stripModifiers(k); //limit to ascii(get rid of a modifier)
		}
	}
	subtractFromReport(k);
	auto tp = std::make_shared<Report>(keyReport);
	bque.push(tp);
	return k;
}


void Macro::keyboardReleaseAll(std::string l)
{
		keyReport.modifiers = 0;
		for (int i = 0; i<NR_OF_KEYS; ++i)
		{
			keyReport.keys[i] = 0;
		}
		auto tp = std::make_shared<Report>(keyReport);
		bque.push(tp);
}

size_t Macro::mousePush(std::string l)
{
		std::istringstream str{ l };
		unsigned int key;
		str >> key;
		mouseReport.buttons |= key;
		mouseMove("0,0,0");
		return 0;
}

void Macro::mouseReleaseAll(std::string l)
{
	mouseReport.buttons = 0;
	mouseMove("0,0,0");
}

size_t Macro::mouseRelease(std::string l)
{
		std::istringstream str{ l };
		unsigned int key;
		str >> key;
		mouseReport.buttons &= ~key;
		mouseMove("0,0,0");
		return key;
}

void Macro::mouseMove(std::string l)
{
		std::istringstream str{ l };
		unsigned int X,Y,angle;
		str >> X>>Y>>angle;
		mouseReport.X = X;
		mouseReport.Y = Y;
		mouseReport.Wheel = angle;
		auto tp = std::make_shared<MouseReport>(mouseReport);
		bque.push(tp);
}

Macro::~Macro()
{
}

void Macro::ReadFileInLongFormat()
{
	while (std::getline(file, buffer))
	{
		lines.push_back(buffer);
	}
}

void Macro::RunLongMacro()
{
	int lineNumber = 1;
	try {
		for (auto &line : lines)
		{
			std::string name;
			std::string args;
			std::istringstream tempBuffer{ line };
			tempBuffer >> name >> args;
			mp[name](args);
			++lineNumber;
		}
	}
	catch (const ParseException &ex)
	{
		throw ParseException{ "Error at line: " + std::to_string(lineNumber) + ": " + std::string{ex.what()} };
	}
	catch (std::out_of_range &ex)
	{
		throw ParseException{ "Error at line: " + std::to_string(lineNumber) + ": " + ex.what() };
	}
}
void Macro::RunShortMacro()
{
	for (auto &token : tokens)
	{
		keyboardPush(token);
		keyboardRelease(token);
	}
}

uint8_t Macro::KeyCodeFromString(std::string s)
{
	uint8_t k;
	if (npr.count(s))
		return npr[s];
	else {
		std::istringstream str{ s };
		str >> k;
	}
	return k;
}

void Macro::ReadFileInShortFormat()
{
	std::stringstream tempBuffer;
	tempBuffer << file.rdbuf();
	boost::split(tokens, tempBuffer.str(), boost::is_any_of("\t\r "), boost::token_compress_on);
}

Format Macro::ParseHeader(std::string header)
{
	boost::trim(header);	
	if (header == "[SHORT_MODE]")
		return Format::Short;
	else if (header == "[LONG_MODE]")
		return Format::Long;
	else
		throw ParseException("Header parsing error, no correct header specified");
}


uint8_t Macro::isAModifier(uint8_t k) {
	return k >= 128;
}
uint8_t Macro::isNonPrintable(uint8_t k) {
	return k >= 136;
}
uint8_t Macro::isPrintableReachedWithShift(uint8_t k) {
	return (k & SHIFT);
}
uint8_t Macro::stripModifiers(uint8_t k)
{
	uint8_t p = k;
	p &= 0x7F;
	return p;
}
KeyReport *Macro::addToReport(uint8_t k)
{
	//check if k is in already pressed
	for (int i = 0; i < NR_OF_KEYS; ++i) {
		if (keyReport.keys[i] == k)
			return &keyReport;
	}
	//add to the first empty
	for (int i = 0; i < NR_OF_KEYS; ++i)
	{
		if (keyReport.keys[i] == 0x00) {
			keyReport.keys[i] = k;
			break;
		}
	}
	return &keyReport;
}
KeyReport* Macro::subtractFromReport(uint8_t k) {
	for (int i = 0; i<NR_OF_KEYS; ++i) {
		if (keyReport.keys[i] == k)
			keyReport.keys[i] = 0x00;
	}
	return &keyReport;
}

void Macro::Run()
{
	try {
		RunLongMacro();
	}
	catch (const std::exception &ex)
	{
		PrintThread{} << ex.what();
		return;
	}
}
