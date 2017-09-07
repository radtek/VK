// VirtualKeyboard.cpp: Okre�la punkt wej�cia dla aplikacji konsoli.
//
#include "stdafx.h"
#include "boost/program_options.hpp" 
#include "IRunMode.h"
#include "FreeRunningMode.h"
#include "BlockingQueue.h"
#include "Macro.h"
#include "TCPThread.h"
#include "CMDArgsParser.h"
#include "ChangeIPMode.h"
#include <string>
#include <iostream>
#include "Parser.h"
#include "RecordMode.h"
#include "ParseException.h"
#include "NoMode.h"

namespace po = boost::program_options;
using namespace std;
std::unique_ptr<IRunMode> SelectRunMode(po::variables_map &vm);
BlockingQueue<std::shared_ptr<IMessage>> bque{ 50 };


int main(int argc, char *argv[])
{
	std::unique_ptr<TCPThread> sendThread;
	std::unique_ptr<IRunMode> runmode;
	CMDArgsParser parser;
	try {
		auto vm = parser.parseCommandLineArguments(argc, argv);
		runmode = parser.SelectRunMode(vm);
		if (runmode->RequiresConnection()) {
			sendThread = std::unique_ptr<TCPThread>{ new TCPThread{ vm["ip"].as<std::string>(),vm["port"].as<std::string>(),bque } };
			while (sendThread->getState() == State::none)
				std::this_thread::sleep_for(1s);
			if (sendThread->getState() == State::disconnected)
			exit(2);
		}
		runmode->Run();
		//if (std::dynamic_cast<ChangeIPMode*>(runmode.get()) != nullptr) exit(0);
		if(sendThread!=nullptr) sendThread->closeConnection();
	}
	catch(ParseException const &ex)
	{
		std::cerr << ex.what() << std::endl;
		exit(2);
	}
	catch (std::exception &ex) {
		std::cerr << ex.what();
		exit(3);
	}
	catch(...)
	{
		exit(3);
	}
    return 0;
}
