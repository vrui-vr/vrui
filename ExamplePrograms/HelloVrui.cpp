#include <Vrui/Application.h>

class HelloVrui:public Vrui::Application
	{
	public:
	HelloVrui(int& argc,char**& argv)
		:Vrui::Application(argc,argv)
		{
		Vrui::showErrorMessage("HelloVrui","Hello, Vrui!","OK");
		}
	};

VRUI_APPLICATION_RUN(HelloVrui)
