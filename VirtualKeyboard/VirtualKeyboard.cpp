// VirtualKeyboard.cpp: Okre�la punkt wej�cia dla aplikacji konsoli.
//

#include "stdafx.h"
#include "boost/program_options.hpp" 
#include "IRunMode.h"
#include "FreeRunningMode.h"
#include "Keyboard.h"
#include "HIDMouse.h"
#include "HIDKeyboard.h"
#include "BlockingQueue.h"
#include "ConfigLoader.h"
#include "Macro.h"
#include "TCPThread.h"
#include "KeyboardTranslator.h"
#include <string>
#include <iostream>


namespace po = boost::program_options;
using namespace std;
IRunMode *runmode;
HIDKeyboard hidkbd;
KeyboardTranslator tr;
Keyboard keyboard{ tr,hidkbd };
HIDMouse hidMouse;
Mouse mouse{hidMouse};
BlockingQueue<std::shared_ptr<IMessage>> bque{ 50 };

void printer(const Report &rep)
{
	auto tp = make_shared<Report>(rep);
	bque.push(tp);
	printf("%i %02x [%02x %02x %02x %02x %02x %02x]\n", rep.id, rep.modifiers, rep.keys[0], rep.keys[1], rep.keys[2], rep.keys[3], rep.keys[4], rep.keys[5]);
}
void mousePrinter(const MouseReport &rep)
{
	printf("id: %i buttons: %i X: %i Y: %i Wheel: %i\n",rep.id,rep.buttons,rep.X,rep.Y,rep.Wheel);
}
int main(int argc, char *argv[])
{
	ConfigLoader cfg;
	Macro *macro=new Macro{bque};
	cfg.load( "config.txt");
	hidkbd.connect(printer);
	mouse.connect(mousePrinter);
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")//done
		("macro,i", po::value<std::string>(), "run given script")
		("record,r", po::value<std::string>(), "record")
		("keyboard,k", "hook keyboard")//done
		("mouse,m", "hook mouse")//done
		("dual,d", "run in dual mode")//done
		("config", po::value<std::string>(), "specifies the config file")//done
		;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	runmode = new FreeRunningMode{};
	runmode->keyhookproc = LowLevelProcNoOp;
	runmode->mousehookproc = LowLevelProcNoOp;
	if (vm.count("config"))
	{
		cfg.load(vm["config"].as<std::string>());
	}
	if (vm.count("help")) {
		cout << desc << "\n";
		return 0;
	}
	if (vm.count("dual"))
	{
		if (vm.count("mouse"))
			runmode->mousehookproc = &LowLevelMouseProcDual;
		if (vm.count("keyboard"))
			runmode->keyhookproc = &LowLevelKeyboardProcDual;
	}
	else {
		if (vm.count("mouse"))
		{
			runmode->mousehookproc = &LowLevelMouseProc;
		}
		if (vm.count("keyboard"))
		{
			runmode->mousehookproc = &LowLevelMouseProc;
		}
	}
	if (vm.count("macro")) {
		try {
			macro->load(vm["macro"].as<std::string>());
		}
		catch (std::exception &ex) {
			cerr << ex.what() << endl;
			return 1;
		}
		runmode = macro;
	}
	else if (vm.count("record"))
	{
		return 0;
	}
	cout << "Script file not specified. Running record mode"<<std::endl;
	//runmode->keyhookproc = LowLevelKeyboardProcDual;
	//runmode->mousehookproc = LowLevelMouseProcDual;
	printf("Connecting...\n");
		TCPThread sendThread{ cfg.options["IP"],cfg.options["PORT"],bque};
		while (sendThread.getState() == State::none);
		if (sendThread.getState() == State::disconnected)
			exit(2);
		else {
			runmode->Run();
		}
	
    return 0;
}

